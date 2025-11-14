#include "rtimvServer.hpp"

rtimvServer::rtimvServer( int argc, char **argv, QObject *Parent ) : QObject( Parent )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    setup( argc, argv );

    if( doHelp )
    {
        help();
        exit( 0 );
    }

    connect( this, SIGNAL( gotConfigure( const configSpec * ) ), this, SLOT( doConfigure( const configSpec * ) ) );

    m_serverThread = QThread::create( [this]() { startServer(); } );
    if( m_serverThread )
    {
        m_serverThread->start();
    }
}

rtimvServer::~rtimvServer()
{
}

void rtimvServer::setupConfig()
{
}

void rtimvServer::loadConfig()
{
}

void rtimvServer::startServer()
{
    int port = 7000;

    std::string server_address = std::format( "0.0.0.0:{}", port );

    grpc::EnableDefaultHealthCheckService( true );

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort( server_address, grpc::InsecureServerCredentials() );

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService( this );

    // Finally assemble the server.
    std::unique_ptr<Server> server( builder.BuildAndStart() );
    std::cout << "Server listening on " << server_address << '\n';

    while( 1 )
    {
        // We need a shared lock b/c we're iterating and accessing the threads
        sharedLockT slock( m_clientMutex );

        auto client = m_clients.begin();
        while( client != m_clients.end() )
        {
            auto imageTh = client->second;

            if( imageTh == nullptr )
            {
                std::cerr << "got nullptr\n";

                // get key, give up shared mutex, get exclusive lock, then delete using key lookup.

                std::string key = client->first;

                slock.unlock();

                uniqueLockT ulock( m_clientMutex );

                client = m_clients.find( key );
                m_clients.erase( client );

                break; // give up mutex and start loop over
            }
            else
            {
                double slr = imageTh->sinceLastRequest();

                if( slr > m_clientDisconnect )
                {
                    // get key,
                    std::string ckey = client->first; // save this b/c it's going away

                    // give up shared mutex,
                    slock.unlock();
                    // so we can get exclusive lock,
                    uniqueLockT ulock( m_clientMutex );
                    // Now reset client in case something changed curing the re-lock
                    client = m_clients.find( ckey );

                    imageTh = client->second;

                    imageTh->quit();

                    if( !imageTh->wait( 1000 ) )
                    {
                        std::cerr << "QThread for client " << ckey << " didn't quit.  Terminating.\n";
                        imageTh->terminate();
                    }

                    imageTh->deleteLater();
                    m_clients.erase( client );

                    std::cerr << "Client " << ckey << " disconnected.\n";

                    break; // give up mutex and start loop over
                }
                else if( slr > m_clientSleep )
                {
                    if( !imageTh->asleep() )
                    {
                        imageTh->emit_gotosleep();
                        std::cerr << "Client " << client->first << " put to sleep\n";
                    }
                }
            }

            ++client;
        }

        if( slock.owns_lock() )
        {
            slock.unlock();
        }

        mx::sys::sleep( 1 );
    }
}

ServerUnaryReactor *rtimvServer::Configure( CallbackServerContext *context, const Config *config, ConfigResult *result )
{
    // We don't lock client mutex here b/c we are just checking count and don't access the values.

    if( m_clients.count( context->peer() ) > 0 )
    {
        std::cerr << "Attempt to reconfigure " << context->peer() << '\n';

        result->set_result( -2 );
    }
    else
    {
        configSpec *cspec = new configSpec( context->peer(), config );
        emit gotConfigure( cspec );

        // Now we wait for configuration to finish

        while( m_clients.count( context->peer() ) == 0 )
        {
            sleep( 1 );
        }

        // now we need a shared lock b/c we access the thread
        sharedLockT lock( m_clientMutex );

        // Now check that it wasn't deleted while we waited for lock:
        if( m_clients.count( context->peer() ) == 0 )
        {
            std::cerr << "thread wires crossed during configure\n";

            ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish( Status::OK );

            return reactor;
        }

        while( m_clients[context->peer()]->configured() == 0 )
        {
            sleep( 1 );
        }

        if( m_clients[context->peer()]->configured() == 1 )
        {
            result->set_result( 0 );
        }
        else
        {
            result->set_result( -1 );
        }
    }

    ServerUnaryReactor *reactor = context->DefaultReactor();
    reactor->Finish( Status::OK );

    return reactor;
}

ServerUnaryReactor *
rtimvServer::ImagePlease( CallbackServerContext *context, const ImageRequest *request, Image *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        std::cerr << "Client " << context->peer() << " not configured\n";

        reply->set_status( remote_rtimv::IMAGE_STATUS_NOT_CONFIGURED );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        reactor->Finish( Status::OK );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_ERROR );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        reactor->Finish( Status::OK ); // maybe send something else?
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        reactor->Finish( Status::OK );
        return reactor;
    }

    int maxWaits;

    if( m_waitSleep <= 0 ) // prevent an infinite busy
    {
        maxWaits = 1;
    }
    else
    {
        maxWaits = m_waitTimeout / ( m_waitSleep / 1000. ) + 1;
    }

    int nwaits = 0;
    while( imageTh->newImage() == false && nwaits < maxWaits )
    {
        mx::sys::milliSleep( m_waitSleep );
        ++nwaits;
    }

    if( imageTh->newImage() == false )
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_TIMEOUT );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        imageTh->lastRequest( -1 ); // sets to now, again b/c of wait

        reactor->Finish( Status::OK );
        return reactor;
    }

    // Now we can render the latest image and send it
    std::string *im = new std::string;

    imageTh->mtxuL_render( im );

    imageTh->lastRequest( -1 ); // sets to now, again b/c of wait

    reply->set_status( remote_rtimv::IMAGE_STATUS_VALID );
    reply->set_atime( imageTh->imageTime() );
    reply->set_fps( imageTh->fpsEst() );

    reply->set_allocated_image( im );

    reactor->Finish( Status::OK );

    return reactor;
}

ServerUnaryReactor *
rtimvServer::GetPixel( CallbackServerContext *context, const Coord *request, Pixel *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        std::cerr << "Client " << context->peer() << " not configured\n";

        reply->set_valid(false);
        reply->set_value(0);
        reactor->Finish( Status::OK );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reply->set_valid(false);
        reply->set_value(0);
        reactor->Finish( Status::OK ); // maybe send something else?
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_valid(false);
        reply->set_value(0);
        reactor->Finish( Status::OK );
        return reactor;
    }

    uint32_t x = request->x();
    uint32_t y = request->y();

    if(x > imageTh->nx() -1 || y > imageTh->ny() - 1)
    {
        reply->set_valid(false);
        reply->set_value(0);
        reactor->Finish( Status::OK ); // maybe send something else?
        return reactor;
    }

    float val = imageTh->calPixel(x,y);

    reply->set_valid(true);
    reply->set_value(val);

    reactor->Finish( Status::OK );
    return reactor;
}


void rtimvServer::doConfigure( const configSpec *cspec )
{
    std::string uri = cspec->m_uri;

    std::shared_ptr<std::vector<std::string>> argv = std::make_shared<std::vector<std::string>>();

    argv->push_back( "rst" );

    if( cspec->m_config.file() != "" )
    {
        argv->push_back( "-c" );
        argv->push_back( cspec->m_config.file() );
    }

    if( cspec->m_config.image_key() != "" )
    {
        argv->push_back( "--image.key" );
        argv->push_back( cspec->m_config.image_key() );
    }

    if( cspec->m_config.dark_key() != "" )
    {
        argv->push_back( "--dark.key" );
        argv->push_back( cspec->m_config.dark_key() );
    }

    if( cspec->m_config.mask_key() != "" )
    {
        argv->push_back( "--mask.key" );
        argv->push_back( cspec->m_config.mask_key() );
    }

    if( cspec->m_config.sat_mask_key() != "" )
    {
        argv->push_back( "--satMask.key" );
        argv->push_back( cspec->m_config.sat_mask_key() );
    }

    /*if( cspec->m_config.update_fps_set() )
    {
        argv->push_back( "--update.fps" );
        argv->push_back( std::to_string( cspec->m_config.update_fps() ) );
    }*/

    if( cspec->m_config.update_timeout_set() )
    {
        argv->push_back( "--update.timeout" );
        argv->push_back( std::to_string( cspec->m_config.update_timeout() ) );
    }

    if( cspec->m_config.update_cube_fps_set() )
    {
        argv->push_back( "--update.cubeFPS" );
        argv->push_back( std::to_string( cspec->m_config.update_cube_fps() ) );
    }

    if( cspec->m_config.autoscale_set() )
    {
        if( cspec->m_config.autoscale() )
        {
            argv->push_back( "--autoscale" );
        }
    }

    if( cspec->m_config.darksub_set() )
    {
        if( cspec->m_config.darksub() )
        {
            argv->push_back( "--darksub" );
        }
    }

    if( cspec->m_config.satlevel_set() )
    {
        argv->push_back( "--satLevel" );
        argv->push_back( std::to_string( cspec->m_config.satlevel() ) );
    }

    if( cspec->m_config.mask_sat_set() )
    {
        if( cspec->m_config.mask_sat() )
        {
            argv->push_back( "--masksat" );
        }
    }

    if( cspec->m_config.mzmq_always_set() )
    {
        if( cspec->m_config.mzmq_always() )
        {
            argv->push_back( "--mzmq.always" );
        }
    }

    if( cspec->m_config.mzmq_server() != "" )
    {
        argv->push_back( "--mzmq.server" );
        argv->push_back( cspec->m_config.mzmq_server() );
    }

    if( cspec->m_config.mzmq_port_set() )
    {
        argv->push_back( "--mzmq.port" );
        argv->push_back( std::to_string(cspec->m_config.mzmq_port()) );
    }

    delete cspec;

    { // mutex scope

        // Get a unique lock on clients to insert
        uniqueLockT ulock( m_clientMutex );

        if( m_clients.count( uri ) > 0 )
        {
            std::cerr << "error: client " << uri << " already configured\n";
            return;
        }

        rtimvServerThread *imageTh = new rtimvServerThread( uri, argv ); //takes ownership of argv

        m_clients.insert( clientT( uri, imageTh ) );

        ///\todo check errors on insert!
    }

    // Now switch to a shared lock for thread access
    sharedLockT slock( m_clientMutex );

    rtimvServerThread *imageTh = m_clients[uri]; // previous imageTh inside mutex scope

    imageTh->configure();

    if( imageTh->configured() == 1 )
    {
        imageTh->start();
        imageTh->lastRequest( -1 ); // sets to now

        std::cerr << "Client " << uri << " configured\n";
    }
    else
    {
        std::cerr << "Client " << uri << " configuration failed\n";
        ///\todo should we delete here?
    }
}
