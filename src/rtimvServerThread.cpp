#include "rtimvServerThread.hpp"

rtimvServerThread::rtimvServerThread( int sockDescrip, const std::string &configFile, QObject *parent )
    : QThread( parent )
{
    std::cerr << "thread: " << configFile << "\n";

    m_tcpSocket = new QTcpSocket;

    if( !m_tcpSocket->setSocketDescriptor(sockDescrip ) )
    {
        std::cerr << "socket error\n";
        emit error( m_tcpSocket->error() );
        return;
    }

    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.


    std::vector<std::string> argvstr( { "rst", "-c", configFile } );
    std::vector<const char *> argv( argvstr.size() + 1, NULL );
    for( size_t index = 0; index < argvstr.size(); ++index )
    {
        argv[index] = argvstr[index].c_str();
    }

    setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

    m_foundation->m_imageTimer.start( m_imageTimeout );

    /*if( doHelp )
    {
        help();
        exit( 0 );
    }*/
}

rtimvServerThread::~rtimvServerThread()
{
}

void rtimvServerThread::run()
{
    std::cerr << "execing\n";

    int rv = exec();

    std::cerr << "execed: " << rv << '\n';

    m_tcpSocket->disconnectFromHost();
    m_tcpSocket->waitForDisconnected();
}

void rtimvServerThread::onConnect()
{
    m_connected = true;
}

void rtimvServerThread::mtxL_postSetImsize( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );
}

void rtimvServerThread::post_zoomLevel()
{
}

template <class lockT>
void rtimvServerThread::mtxL_postRecolorImpl( const lockT &lock )
{
    RTIMV_DEBUG_BREADCRUMB

    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB

    std::cerr << "render\n";
    std::cerr << m_qim->width() << ' ' << m_qim->height() << '\n';
    std::cerr << m_qim->dotsPerMeterX() << ' ' << m_qim->dotsPerMeterY() << '\n';

    //QImage qim = m_qim->scaled(QSize(80,80));

    m_qim->save("render.jpeg", nullptr, 75);

    // Do the jpeg here!

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvServerThread::mtxL_postRecolor( const uniqueLockT &lock )
{
    mtxL_postRecolorImpl( lock );
}

void rtimvServerThread::mtxL_postRecolor( const sharedLockT &lock )
{
    mtxL_postRecolorImpl( lock );
}

void rtimvServerThread::mtxL_postChangeImdata( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB
}

