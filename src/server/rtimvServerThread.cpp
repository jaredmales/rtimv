/** \file rtimvServerThread.cpp
 * \brief Definitions for the rtimvServerThread class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvServerThread.hpp"
#include "rtimvLog.hpp"
#include "rtimvColorGRPC.hpp"
#include "rtimvFilterGRPC.hpp"

#include <QBuffer>
#include <QMetaObject>
#include <algorithm>

namespace
{
std::string image0OrUnknown( rtimvServerThread *imageTh )
{
    if( imageTh == nullptr )
    {
        return "unknown";
    }

    std::string im0 = imageTh->imageName( 0 );
    if( im0.empty() )
    {
        return "unknown";
    }

    return im0;
}

} // namespace

rtimvServerThread::rtimvServerThread( const std::string &uri,
                                      std::shared_ptr<std::vector<std::string>> argv,
                                      int defaultQuality,
                                      const std::string &calledName,
                                      bool includeAppName,
                                      QObject *parent )
    : QThread( parent ), m_uri( uri ), m_defaultQuality( defaultQuality ), m_calledName( calledName ),
      m_includeAppName( includeAppName )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    m_imageTimeout = 100;

    connect( this, SIGNAL( gotosleep() ), this, SLOT( sleep() ) );
    connect( this, SIGNAL( awaken() ), this, SLOT( wakeup() ) );
    connect( this, SIGNAL( servicePending() ), this, SLOT( servicePendingImageRequests() ), Qt::QueuedConnection );

    lastRequest( -1 ); // set to now

    std::cout << rtimv::formatServerLogMessage(
                     m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "thread started" )
              << '\n';

    m_argv = argv;
}

rtimvServerThread::~rtimvServerThread()
{
}

void rtimvServerThread::setupConfig()
{
    rtimvBase::setupConfig();

    config.add( "quality",
                "",
                "quality",
                mx::app::argType::Required,
                "",
                "quality",
                false,
                "int",
                "JPEG transport quality for this image stream.  Range is 0 to 100." );
}

void rtimvServerThread::loadConfig()
{
    rtimvBase::loadConfig();

    m_startupQualitySet = config.isSet( "quality" );
    config( m_startupQuality, "quality" );
    m_startupQuality = std::clamp( m_startupQuality, 0, 100 );
}

void rtimvServerThread::configure()
{
    if( !m_argv )
    {
        std::cerr << rtimv::formatServerLogMessage( m_calledName,
                                                    m_includeAppName,
                                                    m_uri,
                                                    image0OrUnknown( this ),
                                                    "configure failed: missing argv" )
                  << '\n';
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
    std::cout << rtimv::formatServerLogMessage(
                     m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "configuring" )
              << '\n';

    try
    {
        setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        quality( m_defaultQuality );
        if( m_startupQualitySet )
        {
            quality( m_startupQuality );
        }
        m_configured.store( 1, std::memory_order_relaxed );
        m_foundation->m_imageTimer.start( m_imageTimeout );
        std::cout << rtimv::formatServerLogMessage(
                         m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "configured" )
                  << '\n';
    }
    catch( ... )
    {
        std::cerr << rtimv::formatServerLogMessage(
                         m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "configure failed: exception" )
                  << '\n';
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
    std::cout << rtimv::formatServerLogMessage(
                     m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "image connected" )
              << '\n';
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

    markResponseDirty();
}

void rtimvServerThread::mtxL_postRecolor( const sharedLockT &lock )
{
    static_cast<void>( lock );

    markResponseDirty();
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
}

void rtimvServerThread::setImageTimeout( int to )
{
    if( m_foundation == nullptr )
    {
        return;
    }

    if( QThread::currentThread() == m_foundation->thread() )
    {
        m_foundation->imageTimeout( to );
        return;
    }

    QMetaObject::invokeMethod( m_foundation, "imageTimeout", Qt::BlockingQueuedConnection, Q_ARG( int, to ) );
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
        markResponseDirty();
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

void rtimvServerThread::enqueueImageRequest( grpc::CallbackServerContext *context,
                                             grpc::ServerUnaryReactor *reactor,
                                             remote_rtimv::Image *reply )
{
    if( !connected() )
    {
        if( reply != nullptr )
        {
            reply->set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
            std::string *image = new std::string;
            reply->set_allocated_image( image );
        }

        if( reactor != nullptr )
        {
            reactor->Finish( grpc::Status::OK );
        }

        return;
    }

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_pendingImageMutex );

        pendingImageRequest request;
        request.m_context = context;
        request.m_reactor = reactor;
        request.m_reply = reply;
        request.m_enqueueTime = mx::sys::get_curr_time();

        m_pendingImageRequests.push_back( request );
    }

    schedulePendingImageService();
}

void rtimvServerThread::prunePendingImageRequests( double timeoutSeconds )
{
    std::deque<pendingImageRequest> timedOutRequests;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_pendingImageMutex );

        const double now = mx::sys::get_curr_time();
        auto requestIt = m_pendingImageRequests.begin();
        while( requestIt != m_pendingImageRequests.end() )
        {
            if( requestIt->m_context != nullptr && requestIt->m_context->IsCancelled() )
            {
                requestIt = m_pendingImageRequests.erase( requestIt );
                continue;
            }

            if( timeoutSeconds > 0 && now - requestIt->m_enqueueTime > timeoutSeconds )
            {
                timedOutRequests.push_back( *requestIt );
                requestIt = m_pendingImageRequests.erase( requestIt );
                continue;
            }

            ++requestIt;
        }
    }

    for( auto &request : timedOutRequests )
    {
        if( request.m_reply == nullptr || request.m_reactor == nullptr )
        {
            continue;
        }

        std::string *image = new std::string;
        request.m_reply->set_allocated_image( image );

        if( connected() )
        {
            request.m_reply->set_status( remote_rtimv::IMAGE_STATUS_TIMEOUT );
            populateImageReply( request.m_reply );
        }
        else
        {
            request.m_reply->set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
        }

        request.m_reactor->Finish( grpc::Status::OK );
    }
}

size_t rtimvServerThread::pendingImageRequests()
{
    std::lock_guard<std::mutex> lock( m_pendingImageMutex );
    return m_pendingImageRequests.size();
}

void rtimvServerThread::servicePendingImageRequests()
{
    m_servicePendingScheduled.store( false, std::memory_order_relaxed );

    pendingImageRequest request;
    bool haveRequest{ false };

    prunePendingImageRequests( 0 );

    if( !connected() || !m_responseDirty.load( std::memory_order_relaxed ) )
    {
        return;
    }

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_pendingImageMutex );

        if( !m_pendingImageRequests.empty() )
        {
            request = m_pendingImageRequests.front();
            m_pendingImageRequests.pop_front();
            haveRequest = true;
        }
    }

    if( !haveRequest )
    {
        return;
    }

    if( request.m_context != nullptr && request.m_context->IsCancelled() )
    {
        schedulePendingImageService();
        return;
    }

    const uint64_t deliverSerial = m_responseSerial.load( std::memory_order_relaxed );

    std::string *image = new std::string;
    mtxuL_render( image );

    if( request.m_reply != nullptr )
    {
        request.m_reply->set_status( remote_rtimv::IMAGE_STATUS_VALID );
        request.m_reply->set_response_serial( deliverSerial );
        populateImageReply( request.m_reply );
        request.m_reply->set_allocated_image( image );
    }
    else
    {
        delete image;
    }

    if( request.m_reactor != nullptr )
    {
        request.m_reactor->Finish( grpc::Status::OK );
    }

    if( m_responseSerial.load( std::memory_order_relaxed ) == deliverSerial )
    {
        m_responseDirty.store( false, std::memory_order_relaxed );
    }

    lastRequest( -1 );

    if( m_responseDirty.load( std::memory_order_relaxed ) && pendingImageRequests() > 0 )
    {
        schedulePendingImageService();
    }
}

void rtimvServerThread::markResponseDirty()
{
    m_responseSerial.fetch_add( 1, std::memory_order_relaxed );
    m_responseDirty.store( true, std::memory_order_relaxed );

    schedulePendingImageService();
}

void rtimvServerThread::populateImageReply( remote_rtimv::Image *reply )
{
    if( reply == nullptr )
    {
        return;
    }

    reply->set_nx( nx() );
    reply->set_ny( ny() );
    reply->set_nz( nz() );
    reply->set_no( imageNo( 0 ) );

    reply->set_atime( imageTime() );
    reply->set_fps( fpsEst() );

    reply->set_saturated( saturated() );

    reply->set_min_image_data( minImageData() );
    reply->set_max_image_data( maxImageData() );
    reply->set_min_scale_data( minScaleData() );
    reply->set_max_scale_data( maxScaleData() );
    reply->set_source_bytes_per_pixel( bytesPerPixel( 0 ) );

    reply->set_colorbar( rtimv::colorbar2grpc( colorbar() ) );
    reply->set_colormode( rtimv::colormode2grpc( colormode() ) );
    reply->set_colorstretch( rtimv::stretch2grpc( stretch() ) );

    reply->set_autoscale( autoScale() );

    reply->set_subtract_dark( subtractDark() );
    reply->set_apply_mask( applyMask() );
    reply->set_apply_sat_mask( applySatMask() );

    reply->set_image_timeout( imageTimeout() );
    reply->set_cube_dir( cubeDir() );
    reply->set_quality( quality() );

    reply->set_hp_filter( rtimv::hpFilter2grpc( hpFilter() ) );
    reply->set_hpf_fw( hpfFW() );
    reply->set_apply_hp_filter( applyHPFilter() );

    reply->set_lp_filter( rtimv::lpFilter2grpc( lpFilter() ) );
    reply->set_lpf_fw( lpfFW() );
    reply->set_apply_lp_filter( applyLPFilter() );

    reply->set_stats_box( statsBox() );
    reply->set_stats_box_i0( statsBox_i0() );
    reply->set_stats_box_i1( statsBox_i1() );
    reply->set_stats_box_j0( statsBox_j0() );
    reply->set_stats_box_j1( statsBox_j1() );
    reply->set_stats_box_min( statsBox_min() );
    reply->set_stats_box_max( statsBox_max() );
    reply->set_stats_box_mean( statsBox_mean() );
    reply->set_stats_box_median( statsBox_median() );

    reply->set_color_box( colormode() == rtimv::colormode::minmaxbox );
    reply->set_color_box_i0( colorBox_i0() );
    reply->set_color_box_i1( colorBox_i1() );
    reply->set_color_box_j0( colorBox_j0() );
    reply->set_color_box_j1( colorBox_j1() );
    reply->set_color_box_min( colorBox_min() );
    reply->set_color_box_max( colorBox_max() );
}

void rtimvServerThread::schedulePendingImageService()
{
    if( m_servicePendingScheduled.exchange( true, std::memory_order_relaxed ) )
    {
        return;
    }

    emit servicePending();
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
    std::cout << rtimv::formatServerLogMessage(
                     m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "sleeping" )
              << '\n';
}

void rtimvServerThread::wakeup()
{
    m_foundation->m_imageTimer.start();
    m_asleep.store( false, std::memory_order_relaxed );
    std::cout << rtimv::formatServerLogMessage(
                     m_calledName, m_includeAppName, m_uri, image0OrUnknown( this ), "awake" )
              << '\n';
}
