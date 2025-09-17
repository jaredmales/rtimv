#include "rtimvServer.hpp"

rtimvServer::rtimvServer( int argc, char **argv, QObject *Parent ) : QTcpServer( Parent )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    setup( argc, argv );

    if( doHelp )
    {
        help();
        exit( 0 );
    }

    startServer();
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

    if( !listen( QHostAddress::Any, 7000 ) )
    {
        std::cerr << "Unable to start the server: "  << '\n'; //errorString()
        close();
        return;
    }

}

void rtimvServer::incomingConnection(qintptr socketDescriptor)
{
    const std::string configFile = "server.conf";
    rtimvServerThread *thread = new rtimvServerThread(socketDescriptor, configFile, this);
    connect(thread, &rtimvServerThread::finished, thread, &rtimvServerThread::deleteLater);
    thread->start();
}
