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

    connect( this, SIGNAL( gotConfigure( configSpec * ) ), this, SLOT( doConfigure( configSpec * ) ) );

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
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

ServerUnaryReactor *rtimvServer::Configure( CallbackServerContext *context, const Config *config, ConfigResult *result )
{
    if( m_clients.count( context->peer() ) > 0 )
    {
        std::cerr << "Attempt to reconfigure " << context->peer() << '\n';

        result->set_result( -2 );
    }
    else
    {
        configSpec *cspec = new configSpec( context->peer(), config->file() );

        emit gotConfigure( cspec );

        // Now we wait for configuration to finish

        while( m_clients.count( context->peer() ) == 0 )
        {
            sleep( 1 );
        }

        while( m_clients[context->peer()]->m_configured == 0 )
        {
            sleep( 1 );
        }

        if( m_clients[context->peer()]->m_configured == 1 )
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

    if( m_clients.count( context->peer() ) == 0 )
    {
        std::cerr << "Client not configured " << context->peer() << '\n';

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

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        reactor->Finish( Status::OK );
        return reactor;
    }

    float m_waitTimeout = 1;
    int m_waitSleep = 100;

    int maxWaits = m_waitTimeout / ( m_waitSleep / 1000. ) + 1;
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

        reactor->Finish( Status::OK );
        return reactor;
    }

    // Now we can render the latest image and send it
    std::string *im = new std::string;
    imageTh->mtxuL_render( im );

    reply->set_status( remote_rtimv::IMAGE_STATUS_VALID );
    reply->set_atime( imageTh->imageTime() );
    reply->set_fps( imageTh->fpsEst());
    reply->set_allocated_image( im );

    reactor->Finish( Status::OK );

    return reactor;
}

void rtimvServer::doConfigure( configSpec *cspec )
{
    rtimvServerThread *imageTh = new rtimvServerThread( cspec->m_uri, cspec->m_config );
    m_clients.insert( clientT( cspec->m_uri, imageTh ) );

    delete cspec;

    imageTh->configure();

    if( imageTh->m_configured == 1 )
    {
        imageTh->start();
    }
}
