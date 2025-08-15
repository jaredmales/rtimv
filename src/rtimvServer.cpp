#include "rtimvServer.hpp"


rtimvServer::rtimvServer( int argc, char **argv, QWidget *Parent, Qt::WindowFlags f ) : rtimvBase( Parent, f )
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
    config.add( "image.key",
                "",
                "image.key",
                argType::Required,
                "image",
                "key",
                false,
                "string",
                "The main image key. Specifies the protocol, location, and name of the main image." );

    config.add( "dark.key",
                "",
                "dark.key",
                argType::Required,
                "dark",
                "key",
                false,
                "string",
                "The dark image key. Specifies the protocol, location, and name of the dark image." );

    config.add( "mask.key",
                "",
                "mask.key",
                argType::Required,
                "mask",
                "key",
                false,
                "string",
                "The mask image key. Specifies the protocol, location, and name of the mask image." );

    config.add( "satMask.key",
                "",
                "satMask.key",
                argType::Required,
                "satMask",
                "key",
                false,
                "string",
                "The saturation mask image key. Specifies the protocol, location, "
                "and name of the saturation mask image." );

    config.add( "autoscale",
                "",
                "autoscale",
                argType::True,
                "",
                "autoscale",
                false,
                "bool",
                "Set to turn autoscaling on at startup" );

    config.add( "darksub",
                "",
                "darksub",
                argType::True,
                "",
                "darksub",
                false,
                "bool",
                "Set to false to turn off dark subtraction at startup. "
                "If a dark is supplied, darksub is otherwise on." );

    config.add( "satLevel",
                "",
                "satLevel",
                argType::Required,
                "",
                "satLevel",
                false,
                "float",
                "The saturation level for this camera" );

    config.add( "masksat",
                "",
                "masksat",
                argType::True,
                "",
                "masksat",
                false,
                "bool",
                "Set to false to turn off sat-masking at startup. "
                "If a satMaks is supplied, masksat is otherwise on." );

    config.add( "mzmq.always",
                "Z",
                "mzmq.always",
                argType::True,
                "mzmq",
                "always",
                false,
                "bool",
                "Set to make milkzmq the protocol for bare image names.  Note that local shmims can"
                "not be used if this is set." );

    config.add( "mzmq.server",
                "s",
                "mzmq.server",
                argType::Required,
                "mzmq",
                "server",
                false,
                "string",
                "The default server for milkzmq.  The default default is localhost.  This will be overridden by an "
                "image specific server specified in a key." );

    config.add( "mzmq.port",
                "p",
                "mzmq.port",
                argType::Required,
                "mzmq",
                "port",
                false,
                "int",
                "The default port for milkzmq.  The default default is 5556.  This will be overridden by an image "
                "specific port specified in a key." );
}

void rtimvServer::loadConfig()
{
    std::string imKey;
    std::string darkKey;

    std::string flatKey;

    std::string maskKey;

    std::string satMaskKey;

    std::vector<std::string> keys;

    // Set up milkzmq
    config( m_mzmqAlways, "mzmq.always" );
    config( m_mzmqServer, "mzmq.server" );
    config( m_mzmqPort, "mzmq.port" );

    config( imKey, "image.key" );

    config( darkKey, "dark.key" );

    config( maskKey, "mask.key" );

    config( satMaskKey, "satMask.key" );

    // Populate the key vector, a "" means no image specified
    keys.resize( 4 );

    if( imKey != "" )
        keys[0] = imKey;
    if( darkKey != "" )
        keys[1] = darkKey;
    if( maskKey != "" )
        keys[2] = maskKey;
    if( satMaskKey != "" )
        keys[3] = satMaskKey;

    // The command line always overrides the config
    if( config.nonOptions.size() > 0 )
        keys[0] = config.nonOptions[0];
    if( config.nonOptions.size() > 1 )
        keys[1] = config.nonOptions[1];
    if( config.nonOptions.size() > 2 )
        keys[2] = config.nonOptions[2];
    if( config.nonOptions.size() > 3 )
        keys[3] = config.nonOptions[3];

    startup( keys );

    if( m_images[0] == nullptr )
    {
        if( doHelp )
        {
            help();
        }
        else
        {
            std::cerr << "rtimvServer: No valid image specified so cowardly refusing to start.  Use -h for help.\n";
        }

        exit( 0 );
    }

    // Now load remaining options, respecting coded defaults.


    config( m_autoScale, "autoscale" );
    config( m_subtractDark, "darksub" );

    float satLevelDefault = m_satLevel;
    config( m_satLevel, "satLevel" );

    // If we set a sat level or mask, apply it
    if( m_satLevel != satLevelDefault || satMaskKey != "" )
    {
        m_applySatMask = true;
    }

    // except turn it off if requested
    config( m_applySatMask, "masksat" );

}

void rtimvServer::onConnect()
{
    m_connected = true;
}

void rtimvServer::mtxL_postSetImsize( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );
}

void rtimvServer::post_zoomLevel()
{

}

void rtimvServer::mtxL_postRecolor( const uniqueLockT &lock )
{
    mtxL_postRecolorImpl(lock);
}

void rtimvServer::mtxL_postRecolor( const sharedLockT &lock )
{
    mtxL_postRecolorImpl(lock);
}

void rtimvServer::mtxL_postChangeImdata( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvServer::startServer()
{
    m_server = new QTcpServer;

    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));

    m_server->listen(QHostAddress::Any, 7000);
}

void rtimvServer::newConnection()
{
    qDebug() << "Connected\n";
}

