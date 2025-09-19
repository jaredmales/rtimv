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

    connect( this, SIGNAL( gotConfigure( std::string * ) ), this, SLOT( doConfigure( std::string * ) ) );

    m_serverThread = QThread::create([this](){startServer();});
    if(m_serverThread)
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

Status rtimvServer::Configure( ServerContext *context, const Config *config, ConfigResult *result )
{
    std::cout << "Got: " << config->file() << " from: " << context->peer() << '\n';

    std::string *configFile = new std::string( config->file() );

    emit gotConfigure( configFile );

    result->set_result( 0 );
    return Status::OK;
}

void rtimvServer::doConfigure( std::string *configFile )
{
    std::cerr << "doConfigure\n";

    rtimvServerThread *imageTh = new rtimvServerThread( *configFile, this );
    imageTh->start();
}
