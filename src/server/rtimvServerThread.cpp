#include "rtimvServerThread.hpp"
#include <QBuffer>

rtimvServerThread::rtimvServerThread( const std::string &uri, const std::string &configFile, QObject *parent )
    : QThread( parent ), m_uri( uri ), m_configFile( configFile )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.
    
}

rtimvServerThread::~rtimvServerThread()
{
}

void rtimvServerThread::configure()
{

    std::vector<std::string> argvstr( { "rst", "-c", m_configFile } );
    std::vector<const char *> argv( argvstr.size() + 1, NULL );
    for( size_t index = 0; index < argvstr.size(); ++index )
    {
        argv[index] = argvstr[index].c_str();
    }

    m_configured = 0;
    try
    {
        setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        m_configured = 1;
        m_foundation->m_imageTimer.start( m_imageTimeout );
    }
    catch( ... )
    {
        m_configured = -1;
    }
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

    m_qim->save( &buffer, "jpeg", 75 );

    *image = renderedImage.toStdString();

    m_newImage = false;
}

bool rtimvServerThread::newImage()
{
    return m_newImage;
}
