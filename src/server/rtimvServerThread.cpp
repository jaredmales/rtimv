#include "rtimvServerThread.hpp"
#include <QBuffer>

rtimvServerThread::rtimvServerThread( const std::string &uri,
                                      std::shared_ptr<std::vector<std::string>> argv,
                                      QObject *parent )
    : QThread( parent ), m_uri( uri )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    m_imageTimeout = 100;

    connect( this, SIGNAL( gotosleep() ), this, SLOT( sleep() ) );
    connect( this, SIGNAL( awaken() ), this, SLOT( wakeup() ) );

    lastRequest( -1 ); // set to now

    std::cout << "Client: " << uri << " thread started\n";

    m_argv = argv;
}

rtimvServerThread::~rtimvServerThread()
{
}

void rtimvServerThread::configure()
{
    if( !m_argv )
    {
        m_configured.store( -1, std::memory_order_relaxed );
        return;
    }

    std::vector<const char *> argv( m_argv->size() + 1, NULL );
    for( size_t index = 0; index < m_argv->size(); ++index )
    {
        argv[index] = ( *m_argv )[index].c_str();
    }

    m_argv = nullptr;

    m_configured.store( 0, std::memory_order_relaxed );
    try
    {
        setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        m_configured.store( 1, std::memory_order_relaxed );
        m_foundation->m_imageTimer.start( m_imageTimeout );
    }
    catch( ... )
    {
        m_configured.store( -1, std::memory_order_relaxed );
    }
}

int rtimvServerThread::configured()
{
    return m_configured.load( std::memory_order_relaxed );
}

void rtimvServerThread::onConnect()
{
    m_connected = true;
    m_asleep.store( false, std::memory_order_relaxed );
}

void rtimvServerThread::mtxL_postSetImsize( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );
}

void rtimvServerThread::post_zoomLevel()
{
}

void rtimvServerThread::mtxUL_recolor()
{
    sharedLockT lock( m_calMutex );

    mtxL_recolor( lock );
}

void rtimvServerThread::mtxL_postRecolor( const uniqueLockT &lock )
{
    static_cast<void>( lock );

    m_newImage = true;
}

void rtimvServerThread::mtxL_postRecolor( const sharedLockT &lock )
{
    static_cast<void>( lock );

    m_newImage = true;
}

void rtimvServerThread::mtxL_postChangeImdata( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvServerThread::mtxL_postColormode( rtimv::colormode m, const sharedLockT &lock )
{
    static_cast<void>( m );
    static_cast<void>( lock );
}

void rtimvServerThread::mtxuL_render( std::string *image )
{
    sharedLockT lock( m_calMutex );

    QByteArray renderedImage;
    QBuffer buffer( &renderedImage );
    buffer.open( QIODevice::WriteOnly );

    m_qim->save( &buffer, "jpeg", m_quality.load( std::memory_order_relaxed ) );

    *image = renderedImage.toStdString();

    m_newImage.store( false, std::memory_order_relaxed );
}

bool rtimvServerThread::newImage()
{
    return m_newImage.load( std::memory_order_relaxed );
}

int rtimvServerThread::quality()
{
    return m_quality.load( std::memory_order_relaxed );
}

void rtimvServerThread::quality( int q )
{
    if( q < 0 )
    {
        q = 0;
    }
    else if( q > 100 )
    {
        q = 100;
    }

    int oldQ = m_quality.exchange( q, std::memory_order_relaxed );

    if( oldQ != q )
    {
        m_newImage.store( true, std::memory_order_relaxed );
    }
}

double rtimvServerThread::lastRequest()
{
    return m_lastRequest.load( std::memory_order_relaxed );
}

void rtimvServerThread::lastRequest( double lr )
{
    if( lr < 0 )
    {
        lr = mx::sys::get_curr_time();
    }

    m_lastRequest.store( lr, std::memory_order_relaxed );
}

double rtimvServerThread::sinceLastRequest()
{
    double now = mx::sys::get_curr_time();

    return now - m_lastRequest.load( std::memory_order_relaxed );
}

bool rtimvServerThread::asleep()
{
    return m_asleep.load( std::memory_order_relaxed );
}

void rtimvServerThread::rpcBegin()
{
    m_activeRpc.fetch_add( 1, std::memory_order_relaxed );
}

void rtimvServerThread::rpcEnd()
{
    m_activeRpc.fetch_sub( 1, std::memory_order_relaxed );
}

uint32_t rtimvServerThread::rpcActive()
{
    return m_activeRpc.load( std::memory_order_relaxed );
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
    m_asleep.store( true, std::memory_order_relaxed );
}

void rtimvServerThread::wakeup()
{
    m_foundation->m_imageTimer.start();
    m_asleep.store( false, std::memory_order_relaxed );
}
