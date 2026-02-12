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

ServerUnaryReactor *rtimvServer::Configure( CallbackServerContext *context,
                                            const remote_rtimv::Config *config,
                                            remote_rtimv::ConfigResult *result )
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
            // thread wires crossed during configure

            ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish( grpc::Status( grpc::INTERNAL, "sorry" ) );

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

ServerUnaryReactor *rtimvServer::SetColorbar( CallbackServerContext *context,
                                              const remote_rtimv::ColorbarRequest *request,
                                              remote_rtimv::ColorbarResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    rtimv::colorbar cb = rtimv::grpc2colorbar( request->colorbar() );

    if( cb != static_cast<rtimv::colorbar>( -1 ) )
    {
        imageTh->mtxUL_load_colorbar( cb, request->update() );
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color bar" ) );
        return reactor;
    }

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetColormode( CallbackServerContext *context,
                                               const remote_rtimv::ColormodeRequest *request,
                                               remote_rtimv::ColormodeResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    rtimv::colormode cm = rtimv::grpc2colormode( request->colormode() );

    if( cm != static_cast<rtimv::colormode>( -1 ) )
    {
        imageTh->mtxUL_colormode( rtimv::colormode::minmaxglobal );
        reactor->Finish( Status::OK );
        return reactor;
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color mode" ) );
        return reactor;
    }
}

ServerUnaryReactor *rtimvServer::SetColorstretch( CallbackServerContext *context,
                                                  const remote_rtimv::ColorstretchRequest *request,
                                                  remote_rtimv::ColorstretchResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    rtimv::stretch cs = rtimv::grpc2stretch( request->colorstretch() );

    if( cs != static_cast<rtimv::stretch>( -1 ) )
    {
        imageTh->stretch( cs );
        reactor->Finish( Status::OK );
        return reactor;
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color stretch" ) );
        return reactor;
    }
}

ServerUnaryReactor *rtimvServer::SetMinScale( CallbackServerContext *context,
                                              const remote_rtimv::ScaleRequest *request,
                                              remote_rtimv::ScaleResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        grpc::Status status( grpc::FAILED_PRECONDITION, "not configured" );
        reactor->Finish( status );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    imageTh->minScaleData( request->value() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetMaxScale( CallbackServerContext *context,
                                              const remote_rtimv::ScaleRequest *request,
                                              remote_rtimv::ScaleResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    imageTh->maxScaleData( request->value() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::Restretch( CallbackServerContext *context,
                                            const remote_rtimv::RestretchRequest *request,
                                            remote_rtimv::RestretchResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    imageTh->mtxUL_reStretch();

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::ImagePlease( CallbackServerContext *context,
                                              const remote_rtimv::ImageRequest *request,
                                              remote_rtimv::Image *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) ); // maybe send something else?
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
    reply->set_nx( imageTh->nx() );
    reply->set_ny( imageTh->ny() );
    reply->set_nz( imageTh->nz() );
    reply->set_no( imageTh->imageNo( 0 ) );

    reply->set_atime( imageTh->imageTime() );
    reply->set_fps( imageTh->fpsEst() );

    reply->set_allocated_image( im );

    reply->set_saturated( imageTh->saturated() );

    reply->set_min_image_data( imageTh->minImageData() );
    reply->set_max_image_data( imageTh->maxImageData() );
    reply->set_min_scale_data( imageTh->minScaleData() );
    reply->set_max_scale_data( imageTh->maxScaleData() );

    reply->set_colorbar( rtimv::colorbar2grpc( imageTh->colorbar() ) );

    reply->set_colormode( rtimv::colormode2grpc( imageTh->colormode() ) );

    reply->set_colorstretch( rtimv::stretch2grpc( imageTh->stretch() ) );

    reply->set_subtract_dark( imageTh->subtractDark() );
    reply->set_apply_mask( imageTh->applyMask() );
    reply->set_apply_sat_mask( imageTh->applySatMask() );

    reactor->Finish( Status::OK );

    return reactor;
}

ServerUnaryReactor *
rtimvServer::GetPixel( CallbackServerContext *context, const remote_rtimv::Coord *request, remote_rtimv::Pixel *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) ); // maybe send something else?
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
        reply->set_valid( false );
        reply->set_value( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    uint32_t x = request->x();
    uint32_t y = request->y();

    if( x > imageTh->nx() - 1 || y > imageTh->ny() - 1 )
    {
        reply->set_valid( false );
        reply->set_value( 0 );
        reactor->Finish( Status::OK ); // maybe send something else?
        return reactor;
    }

    float val = imageTh->calPixel( x, y );

    reply->set_valid( true );
    reply->set_value( val );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::ColorBox( CallbackServerContext *context,
                                           const remote_rtimv::Box *request,
                                           remote_rtimv::MinvalMaxval *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) ); // maybe send something else?
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
        reply->set_valid( false );
        reply->set_min( 0 );
        reply->set_max( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->colorBox_i0( request->upper_left().x() );
    imageTh->colorBox_j0( request->upper_left().y() );

    imageTh->colorBox_i1( request->lower_right().x() );
    imageTh->colorBox_j1( request->lower_right().y() );

    imageTh->mtxUL_colormode( rtimv::colormode::minmaxbox );

    reply->set_valid( true );
    reply->set_min( imageTh->colorBox_min() );
    reply->set_max( imageTh->colorBox_max() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::Recolor( CallbackServerContext *context,
                                          const remote_rtimv::RecolorRequest *request,
                                          remote_rtimv::RecolorResponse *reply )
{
    ServerUnaryReactor *reactor = context->DefaultReactor();

    // We need a shared lock b/c we access the thread
    sharedLockT slock( m_clientMutex );

    if( m_clients.count( context->peer() ) == 0 )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = m_clients[context->peer()];

    if( imageTh == nullptr ) // Something has gone wrong. Here we expect the client to reconnect
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 ); // sets to now

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
        std::cerr << "Client " << context->peer() << " woken up\n";
    }

    imageTh->mtxUL_recolor();

    reactor->Finish( Status::OK ); // maybe send something else?
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
        argv->push_back( std::to_string( cspec->m_config.mzmq_port() ) );
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

        rtimvServerThread *imageTh = new rtimvServerThread( uri, argv ); // takes ownership of argv

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
