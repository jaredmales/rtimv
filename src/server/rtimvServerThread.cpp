#include "rtimvServerThread.hpp"
#include <QBuffer>

rtimvServerThread::rtimvServerThread( const std::string &uri, const std::string &configFile, QObject *parent )
    : QThread( parent ), m_uri( uri )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    m_imageTimeout = 100;

    connect(this, SIGNAL(gotosleep()), this, SLOT(sleep()));
    connect(this, SIGNAL(awaken()), this, SLOT(wakeup()));

    lastRequest(-1); //set to now

    std::cerr << "I am: " << uri << " with " << configFile << '\n';

    m_argv = new std::vector<std::string>( { "rst", "-c", configFile } );

}

rtimvServerThread::~rtimvServerThread()
{
    if(m_argv)
    {
        delete m_argv;
    }
}

void rtimvServerThread::configure()
{
    if(!m_argv)
    {
    }

    std::vector<const char *> argv( m_argv->size() + 1, NULL );
    for( size_t index = 0; index < m_argv->size(); ++index )
    {
        argv[index] = (*m_argv)[index].c_str();
    }

    m_configured = 0;
    try
    {
        setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        m_configured = 1;
        m_foundation->m_imageTimer.start( m_imageTimeout );

        //Here we're done with it.
        delete m_argv;
        m_argv = nullptr;
    }
    catch( ... )
    {
        m_configured = -1;
    }

    
}

void rtimvServerThread::onConnect()
{
    m_connected = true;
    m_asleep = false;
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
    static_cast<void>( lock );

    m_newImage = true;
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

void rtimvServerThread::mtxL_postSetColorBoxActive( bool usba, const sharedLockT &lock )
{
    static_cast<void>( usba );
    static_cast<void>( lock );
}

void rtimvServerThread::mtxuL_render( std::string *image )
{
    sharedLockT lock( m_calMutex );

    QByteArray renderedImage;
    QBuffer buffer( &renderedImage );
    buffer.open( QIODevice::WriteOnly );

    int m_quality = 50;
    m_qim->save( &buffer, "jpeg", m_quality );

    *image = renderedImage.toStdString();

    m_newImage = false;
}

bool rtimvServerThread::newImage()
{
    return m_newImage;
}

int rtimvServerThread::quality()
{
    return m_quality;
}

void rtimvServerThread::quality( int q )
{
    m_quality = q;
}

double rtimvServerThread::lastRequest()
{
    return m_lastRequest;
}

void rtimvServerThread::lastRequest( double lr )
{
    if( lr < 0 )
    {
        lr = mx::sys::get_curr_time();
    }

    m_lastRequest = lr;
}

double rtimvServerThread::sinceLastRequest()
{
    double now = mx::sys::get_curr_time();

    return now - m_lastRequest;
}

bool rtimvServerThread::asleep()
{
    return m_asleep;
}

void rtimvServerThread::emit_gotosleep()
{
    emit gotosleep();
}

void rtimvServerThread::emit_awaken()
{
    emit awaken();
}

void rtimvServerThread::sleep()
{
    m_foundation->m_imageTimer.stop();
    m_asleep = true;
}

void rtimvServerThread::wakeup()
{
    m_foundation->m_imageTimer.start();
    m_asleep = false;
}
