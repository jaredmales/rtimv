/** \file rtimvClientBase.cpp
 * \brief Definitions for the rtimvClientBase class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvClientBase.hpp"
#include "rtimvLog.hpp"
#include "rtimvColorGRPC.hpp"
#include "rtimvFilterGRPC.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <thread>

#include <QMetaObject>
#include <QPointer>
#include <QCoreApplication>

// #define RTIMV_DEBUG_BREADCRUMB std::cerr << __FILE__ << " " << __LINE__ << "\n";
#define RTIMV_DEBUG_BREADCRUMB

#define SHARED_CONN_LOCK_RET( rv )                                                                                     \
    sharedLockT connLock( m_connectedMutex );                                                                          \
    if( !m_connected )                                                                                                 \
    {                                                                                                                  \
        return rv;                                                                                                     \
    }

#define SHARED_CONN_LOCK SHARED_CONN_LOCK_RET( static_cast<void>( 0 ) )

#define REPORT_SERVER_DISCONNECTED                                                                                     \
    uint64_t connections = m_connections;                                                                              \
    connLock.unlock();                                                                                                 \
                                                                                                                       \
    uniqueLockT lock( m_connectedMutex );                                                                              \
                                                                                                                       \
    if( status.error_code() != grpc::StatusCode::DEADLINE_EXCEEDED && connections == m_connections )                   \
    {                                                                                                                  \
        m_connected = false;                                                                                           \
                                                                                                                       \
        std::cerr << formatBaseLogMessage( std::string( "Message from rtimvServer: " ) + status.error_message() )      \
                  << '\n';                                                                                             \
    }

rtimvClientBase::rtimvClientBase()
{
    m_foundation = new rtimvBaseObject( this, nullptr );
    m_foundation->m_connectionTimer.start( 1000 );
}

rtimvClientBase::~rtimvClientBase()
{
    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        m_shuttingDown = true;

        if( m_ImagePleaseContext )
        {
            m_ImagePleaseContext->TryCancel();
        }
    }

    { // mutex scope
        std::unique_lock<std::mutex> lock( m_imageRequestMutex );
        while( m_imageRequestPending )
        {
            if( m_imageRequestCv.wait_for( lock, std::chrono::seconds( 1 ) ) == std::cv_status::timeout )
            {
                std::cerr << formatBaseLogMessage( "waiting for ImagePlease callback during shutdown." ) << '\n';
            }
        }
    }

    { // mutex scope
        std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );

        if( m_GetPixelContext )
        {
            m_GetPixelContext->TryCancel();
        }

        if( m_ColorBoxContext )
        {
            m_ColorBoxContext->TryCancel();
        }

        if( m_StatsBoxContext )
        {
            m_StatsBoxContext->TryCancel();
        }

        if( m_SetColorstretchContext )
        {
            m_SetColorstretchContext->TryCancel();
        }

        if( m_SetMinScaleContext )
        {
            m_SetMinScaleContext->TryCancel();
        }

        if( m_SetMaxScaleContext )
        {
            m_SetMaxScaleContext->TryCancel();
        }

        for( auto *context : m_emptyRpcContexts )
        {
            if( context )
            {
                context->TryCancel();
            }
        }
    }

    { // mutex scope
        std::unique_lock<std::mutex> lock( m_asyncRpcMutex );

        while( m_getPixelPending || m_colorBoxPending || m_statsBoxPending || m_setColorstretchPending ||
               m_setMinScalePending || m_setMaxScalePending || m_emptyRpcPending > 0 )
        {
            if( m_asyncRpcCv.wait_for( lock, std::chrono::seconds( 1 ) ) == std::cv_status::timeout )
            {
                std::cerr << formatBaseLogMessage( "waiting for async unary RPC callbacks during shutdown." ) << '\n';
            }
        }
    }

    if( m_configReq )
    {
        delete m_configReq;
    }

    if( m_foundation )
    {
        m_foundation->deleteLater();
    }
}

void rtimvClientBase::setupConfig()
{
    config.add( "server",
                "S",
                "server",
                mx::app::argType::Required,
                "",
                "server",
                false,
                "string",
                "IP address of the rtimv grpc server." );

    config.add(
        "port", "P", "port", mx::app::argType::Required, "", "port", false, "int", "Port of the rtimv grpc server." );

    config.add( "image.key",
                "",
                "image.key",
                mx::app::argType::Required,
                "image",
                "key",
                false,
                "string",
                "The main image key. Specifies the protocol, location, and name of the main image." );

    config.add( "dark.key",
                "",
                "dark.key",
                mx::app::argType::Required,
                "dark",
                "key",
                false,
                "string",
                "The dark image key. Specifies the protocol, location, and name of the dark image." );

    config.add( "mask.key",
                "",
                "mask.key",
                mx::app::argType::Required,
                "mask",
                "key",
                false,
                "string",
                "The mask image key. Specifies the protocol, location, and name of the mask image." );

    config.add( "satMask.key",
                "",
                "satMask.key",
                mx::app::argType::Required,
                "satMask",
                "key",
                false,
                "string",
                "The saturation mask image key. Specifies the protocol, location, "
                "and name of the saturation mask image." );

    config.add( "update.fps",
                "",
                "update.fps",
                mx::app::argType::Required,
                "update",
                "fps",
                false,
                "real",
                "Specify the image update timeout in FPS.  Overridden by update.timeout if set." );

    config.add( "update.timeout",
                "",
                "update.timeout",
                mx::app::argType::Required,
                "update",
                "timeout",
                false,
                "real",
                "Specify the image update timeout in ms.  Default is 50 ms (20 FPS). Overrides update.fps." );

    config.add( "update.cubeFPS",
                "",
                "update.cubeFPS",
                mx::app::argType::Required,
                "update",
                "cubeFPS",
                false,
                "real",
                "Specify the image cube update rate in FPS.  Default is 20 FPS." );

    config.add( "colorbar",
                "",
                "colorbar",
                mx::app::argType::Required,
                "",
                "colorbar",
                false,
                "string",
                "Set the startup colorbar. Valid values are grey, jet, hot, heat, bb, bone, red, green, and blue." );

    config.add( "quality",
                "",
                "quality",
                mx::app::argType::Required,
                "",
                "quality",
                false,
                "int",
                "JPEG transport quality for this remote image stream.  Range is 0 to 100." );

    config.add( "update.rollingStatsFrames",
                "",
                "update.rollingStatsFrames",
                mx::app::argType::Required,
                "update",
                "rollingStatsFrames",
                false,
                "int",
                "Specify the number of frames used for rolling averages of compression ratio and frame rate. "
                "Default is 10." );

    config.add( "autoscale",
                "",
                "autoscale",
                mx::app::argType::True,
                "",
                "autoscale",
                false,
                "bool",
                "Set to turn autoscaling on at startup" );

    config.add( "darksub",
                "",
                "darksub",
                mx::app::argType::True,
                "",
                "darksub",
                false,
                "bool",
                "Set to false to turn off dark subtraction at startup. "
                "If a dark is supplied, darksub is otherwise on." );

    config.add( "satLevel",
                "",
                "satLevel",
                mx::app::argType::Required,
                "",
                "satLevel",
                false,
                "float",
                "The saturation level for this camera" );

    config.add( "masksat",
                "",
                "masksat",
                mx::app::argType::True,
                "",
                "masksat",
                false,
                "bool",
                "Set to false to turn off sat-masking at startup. "
                "If a satMaks is supplied, masksat is otherwise on." );

    config.add( "mzmq.always",
                "Z",
                "mzmq.always",
                mx::app::argType::True,
                "mzmq",
                "always",
                false,
                "bool",
                "Set to make milkzmq the protocol for bare image names.  Note that local shmims can"
                "not be used if this is set." );

    config.add( "mzmq.server",
                "s",
                "mzmq.server",
                mx::app::argType::Required,
                "mzmq",
                "server",
                false,
                "string",
                "The default server for milkzmq.  The default default is localhost.  This will be overridden by an "
                "image specific server specified in a key." );

    config.add( "mzmq.port",
                "p",
                "mzmq.port",
                mx::app::argType::Required,
                "mzmq",
                "port",
                false,
                "int",
                "The default port for milkzmq.  The default default is 5556.  This will be overridden by an image "
                "specific port specified in a key." );

    config.add( "log.appname",
                "",
                "log.appname",
                mx::app::argType::Required,
                "log",
                "appname",
                false,
                "bool",
                "Set true/false to include/exclude called-name in log prefixes." );

    config.add( "no-log-appname",
                "",
                "no-log-appname",
                mx::app::argType::True,
                "log",
                "no-appname",
                false,
                "bool",
                "Disable called-name in log prefixes." );
}

void rtimvClientBase::loadConfig()
{
    std::vector<std::string> imageNames( 4 );

    if( m_configReq )
    {
        delete m_configReq;
    }

    m_configReq = new remote_rtimv::Config;

    std::string configFile;
    config( configFile, "config" );
    if( configFile != "" )
    {
        m_configReq->set_file( configFile );
    }

    config( m_server, "server" );
    config( m_port, "port" );

    m_calledName = QCoreApplication::applicationName().toStdString();
    if( m_calledName.empty() )
    {
        m_calledName = "rtimvClient";
    }

    if( config.isSet( "log.appname" ) )
    {
        config( m_logAppName, "log.appname" );
    }

    if( config.isSet( "no-log-appname" ) )
    {
        bool noLogAppName = false;
        config( noLogAppName, "no-log-appname" );
        if( noLogAppName )
        {
            m_logAppName = false;
        }
    }

    std::string imKey;
    config( imKey, "image.key" );
    if( imKey != "" )
    {
        m_configReq->set_image_key( imKey );
        imageNames[0] = imKey;
    }

    std::string darkKey;
    config( darkKey, "dark.key" );
    if( darkKey != "" )
    {
        m_configReq->set_dark_key( darkKey );
        imageNames[1] = darkKey;
    }

    std::string flatKey;
    config( flatKey, "flat.key" );
    if( flatKey != "" )
    {
        m_configReq->set_flat_key( flatKey );
    }

    std::string maskKey;
    config( maskKey, "mask.key" );
    if( maskKey != "" )
    {
        m_configReq->set_mask_key( maskKey );
        imageNames[2] = maskKey;
    }

    std::string satMaskKey;
    config( satMaskKey, "satMask.key" );
    if( satMaskKey != "" )
    {
        m_configReq->set_sat_mask_key( satMaskKey );
        imageNames[3] = satMaskKey;
    }

    // The command line always overrides the config
    if( config.nonOptions.size() > 0 )
    {
        m_configReq->set_image_key( config.nonOptions[0] );
        imageNames[0] = config.nonOptions[0];
    }

    if( config.nonOptions.size() > 1 )
    {
        m_configReq->set_dark_key( config.nonOptions[1] );
        imageNames[1] = config.nonOptions[1];
        // darkKey = config.nonOptions[1]
    }

    if( config.nonOptions.size() > 2 )
    {
        m_configReq->set_mask_key( config.nonOptions[2] );
        imageNames[2] = config.nonOptions[2];
    }

    if( config.nonOptions.size() > 3 )
    {
        m_configReq->set_flat_key( config.nonOptions[3] );
    }

    if( config.nonOptions.size() > 4 )
    {
        m_configReq->set_sat_mask_key( config.nonOptions[4] );
        imageNames[3] = config.nonOptions[4];
    }

    m_imageNames = imageNames;

    if( m_configReq->file() == "" && m_configReq->image_key() == "" )
    {
        if( doHelp )
        {
            help();

            throw std::runtime_error( "help" );
        }
        else
        {
            throw std::runtime_error( "rtimv: No remote config file or valid image specified, so chickening out and "
                                      "refusing to start.  Use -h for help." );
        }
    }

    // Now load remaining options, respecting coded defaults.

    // get timeouts.
    if( config.isSet( "update.fps" ) )
    {
        float fps = -999;
        config( fps, "update.fps" );

        if( fps > 0 ) // fps sets m_imageTimeout
        {
            m_configReq->set_update_timeout( std::round( 1000. / fps ) );
            m_configReq->set_update_timeout_set( true );
        }
    }

    // but update.timeout can override it
    if( config.isSet( "update.timeout" ) )
    {
        uint32_t updateTimeout;
        config( updateTimeout, "update.timeout" );
        m_configReq->set_update_timeout( updateTimeout );
        m_configReq->set_update_timeout_set( true );
    }

    if( config.isSet( "update.cubeFPS" ) )
    {
        float cubeFPS;
        config( cubeFPS, "update.cubeFPS" );
        m_configReq->set_update_cube_fps( cubeFPS );
        m_configReq->set_update_cube_fps_set( true );
    }

    if( config.isSet( "colorbar" ) )
    {
        std::string colorbarName;
        config( colorbarName, "colorbar" );
        std::transform( colorbarName.begin(),
                        colorbarName.end(),
                        colorbarName.begin(),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

        rtimv::colorbar cb;
        if( !rtimv::colorbarFromString( cb, colorbarName ) )
        {
            throw std::runtime_error( "rtimvClient: invalid colorbar '" + colorbarName + "'" );
        }

        m_configReq->set_colorbar( rtimv::colorbar2grpc( cb ) );
        m_configReq->set_colorbar_set( true );
    }

    if( config.isSet( "quality" ) )
    {
        int quality;
        config( quality, "quality" );
        quality = std::clamp( quality, 0, 100 );
        m_configReq->set_quality( quality );
        m_configReq->set_quality_set( true );
    }

    if( config.isSet( "update.rollingStatsFrames" ) )
    {
        config( m_rollingStatsFrames, "update.rollingStatsFrames" );
        if( m_rollingStatsFrames < 1 )
        {
            m_rollingStatsFrames = 1;
        }
    }

    if( config.isSet( "autoscale" ) )
    {
        bool autoscale;
        config( autoscale, "autoscale" );
        m_configReq->set_autoscale( autoscale );
        m_configReq->set_autoscale_set( true );
    }

    if( config.isSet( "darksub" ) )
    {
        bool darksub;
        config( darksub, "darksub" );
        m_configReq->set_darksub( darksub );
        m_configReq->set_darksub_set( true );
    }

    if( config.isSet( "satLevel" ) )
    {
        float satlevel;
        config( satlevel, "satLevel" );
        m_configReq->set_satlevel( satlevel );
        m_configReq->set_satlevel_set( true );
    }

    if( config.isSet( "masksat" ) )
    {
        bool masksat;
        config( masksat, "masksat" );
        m_configReq->set_mask_sat( masksat );
        m_configReq->set_mask_sat_set( true );
    }

    // Set up milkzmq

    if( config.isSet( "mzmq.always" ) )
    {
        bool mzmqalways;
        config( mzmqalways, "mzmq.always" );
        m_configReq->set_mzmq_always( mzmqalways );
        m_configReq->set_mzmq_always_set( true );
    }

    if( config.isSet( "mzmq.server" ) )
    {
        std::string mzmqServer;
        config( mzmqServer, "mzmq.server" );
        m_configReq->set_mzmq_server( mzmqServer );
    }

    if( config.isSet( "mzmq.port" ) )
    {
        uint32_t port;
        config( port, "mzmq.port" );
        m_configReq->set_mzmq_port( port );
        m_configReq->set_mzmq_port_set( true );
    }

    std::string target_str = std::format( "{}:{}", m_server, m_port );

    std::shared_ptr<grpc::Channel> channel( grpc::CreateChannel( target_str, grpc::InsecureChannelCredentials() ) );
    stub_ = remote_rtimv::rtimv::NewStub( channel );
}

void rtimvClientBase::startup()
{
    if( m_foundation )
    {
        // Defer initial connection so window construction is never blocked by Configure().
        QMetaObject::invokeMethod( m_foundation, "reconnect", Qt::QueuedConnection );
    }
}

bool rtimvClientBase::connected()
{
    return m_connected;
}

std::string rtimvClientBase::logImage0() const
{
    if( m_imageNames.size() > 0 && !m_imageNames[0].empty() )
    {
        return m_imageNames[0];
    }

    if( m_configReq && !m_configReq->image_key().empty() )
    {
        return m_configReq->image_key();
    }

    return "unknown";
}

std::string rtimvClientBase::formatBaseLogMessage( std::string_view message ) const
{
    rtimv::logContext ctx;
    ctx.calledName = m_calledName;
    ctx.image0 = logImage0();
    ctx.clientId = m_clientId;
    ctx.includeAppName = m_logAppName;
    ctx.includeClient = true;

    return rtimv::formatLogMessage( ctx, message );
}

void rtimvClientBase::reconnect()
{
    sharedLockT lock( m_connectedMutex );

    if( m_connected )
    {
        return;
    }

    lock.unlock();

    bool shouldConfigure = true;
    if( stub_ )
    {
        remote_rtimv::ImageNameRequest request;
        remote_rtimv::ImageNameResponse response;
        grpc::ClientContext context;
        request.set_image( 0 );

        grpc::Status status = stub_->GetImageName( &context, request, &response );
        if( status.ok() )
        {
            uniqueLockT ulock( m_connectedMutex );
            m_connected = true;
            m_connectionFailReported = false;
            shouldConfigure = false;
        }
        else if( status.error_code() != grpc::StatusCode::FAILED_PRECONDITION )
        {
            // Transport/server error; keep disconnected and let timer retry.
            shouldConfigure = false;
        }
    }

    if( shouldConfigure )
    {
        Configure();
    }

    lock.lock();

    // start getting images again
    if( m_connected )
    {
        m_foundation->emit_ImageNeeded();
    }
    else
    {
        updateAge();
    }
}

void rtimvClientBase::Configure()
{
    uniqueLockT lock( m_connectedMutex );
    ++m_connections;

    if( !m_configReq )
    {
        m_connected = false;
        return;
    }

    remote_rtimv::ConfigResult result;
    grpc::ClientContext context;

    grpc::Status status = stub_->Configure( &context, *m_configReq, &result );

    // Act upon its status.
    if( status.ok() )
    {
        m_clientId = result.client_id();
        m_connected = true;
        updateImageNamesFromServer();
        std::cerr << formatBaseLogMessage( std::format( "connected to {}:{}", m_server, m_port ) ) << '\n';
        m_connectionFailReported = false;
    }
    else
    {
        m_clientId.clear();
        m_connected = false;
        if( !m_connectionFailReported )
        {
            std::cerr << formatBaseLogMessage( status.error_message() ) << '\n';
            m_connectionFailReported = true;
        }
    }
}

void rtimvClientBase::updateImageNamesFromServer()
{
    if( !stub_ )
    {
        return;
    }

    if( m_imageNames.size() < 4 )
    {
        m_imageNames.resize( 4 );
    }

    for( uint32_t n = 0; n < 4; ++n )
    {
        remote_rtimv::ImageNameRequest request;
        remote_rtimv::ImageNameResponse response;
        grpc::ClientContext context;

        request.set_image( n );

        grpc::Status status = stub_->GetImageName( &context, request, &response );
        if( status.ok() && response.valid() )
        {
            if( m_imageNames[n] == "" )
            {
                m_imageNames[n] = response.name();
            }
        }
    }
}

void rtimvClientBase::ImagePlease()
{
    updateAge();

    SHARED_CONN_LOCK

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( m_shuttingDown )
        {
            return;
        }

        if( m_imageRequestPending )
        {

            std::cerr << formatBaseLogMessage( std::format(
                             "bug: in ImagePlease but imageRequestPending is true {} {}", __FILE__, __LINE__ ) )
                      << '\n';
            return;
        }

        if( m_ImagePleaseContext )
        {
            std::cerr << formatBaseLogMessage( std::format(
                             "bug: in ImagePlease but ImagePleaseContext is allocated {} {}", __FILE__, __LINE__ ) )
                      << '\n';
            return;
        }

        m_ImagePleaseContext = new grpc::ClientContext;
        m_ImagePleaseContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 10000 ) );

        m_imageRequestPending = true;
    }

    connLock.unlock();
    dispatchImagePleaseAsync();
}

void rtimvClientBase::dispatchImagePleaseAsync()
{
    stub_->async()->ImagePlease( m_ImagePleaseContext,
                                 &m_grpcImageRequest,
                                 &m_grpcImage,
                                 [this]( grpc::Status status ) { this->ImagePlease_callback( status ); } );
}

void rtimvClientBase::requestPixelValue( uint32_t x, uint32_t y )
{
    sharedLockT connLock( m_connectedMutex );

    if( !m_connected )
    {
        if( m_foundation )
        {
            m_foundation->emit_pixelValueUpdated( x, y, 0, false );
        }
        return;
    }

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    m_getPixelX = x;
    m_getPixelY = y;

    if( m_getPixelPending )
    {
        m_getPixelQueued = true;
        return;
    }

    if( m_GetPixelContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in requestPixelValue but GetPixelContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_getPixelRequest.set_x( m_getPixelX );
    m_getPixelRequest.set_y( m_getPixelY );
    m_getPixelInflightX = m_getPixelX;
    m_getPixelInflightY = m_getPixelY;

    m_GetPixelContext = new grpc::ClientContext;
    m_GetPixelContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_getPixelPending = true;

    stub_->async()->GetPixel( m_GetPixelContext,
                              &m_getPixelRequest,
                              &m_getPixelReply,
                              [this]( grpc::Status status ) { this->GetPixel_callback( status ); } );
}

void rtimvClientBase::requestColorBoxValues()
{
    sharedLockT connLock( m_connectedMutex );

    if( !m_connected )
    {
        if( m_foundation )
        {
            m_foundation->emit_colorBoxUpdated(
                m_colorBox_i0, m_colorBox_i1, m_colorBox_j0, m_colorBox_j1, 0, 0, false );
        }
        return;
    }

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    if( m_colorBoxPending )
    {
        m_colorBoxQueued = true;
        return;
    }

    if( m_ColorBoxContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in requestColorBoxValues but ColorBoxContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_colorBoxRequest.mutable_upper_left()->set_x( m_colorBox_i0 );
    m_colorBoxRequest.mutable_upper_left()->set_y( m_colorBox_j0 );
    m_colorBoxRequest.mutable_lower_right()->set_x( m_colorBox_i1 );
    m_colorBoxRequest.mutable_lower_right()->set_y( m_colorBox_j1 );

    m_colorBoxInflight_i0 = m_colorBox_i0;
    m_colorBoxInflight_i1 = m_colorBox_i1;
    m_colorBoxInflight_j0 = m_colorBox_j0;
    m_colorBoxInflight_j1 = m_colorBox_j1;

    m_ColorBoxContext = new grpc::ClientContext;
    m_ColorBoxContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_colorBoxPending = true;

    stub_->async()->ColorBox( m_ColorBoxContext,
                              &m_colorBoxRequest,
                              &m_colorBoxReply,
                              [this]( grpc::Status status ) { this->ColorBox_callback( status ); } );
}

void rtimvClientBase::requestStatsBoxValues()
{
    sharedLockT connLock( m_connectedMutex );

    if( !m_connected )
    {
        if( m_foundation )
        {
            m_foundation->emit_statsBoxUpdated(
                m_statsBox_i0, m_statsBox_i1, m_statsBox_j0, m_statsBox_j1, 0, 0, 0, 0, false );
        }
        return;
    }

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    if( m_statsBoxPending )
    {
        m_statsBoxQueued = true;
        return;
    }

    if( m_StatsBoxContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in requestStatsBoxValues but StatsBoxContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_statsBoxRequest.mutable_upper_left()->set_x( m_statsBox_i0 );
    m_statsBoxRequest.mutable_upper_left()->set_y( m_statsBox_j0 );
    m_statsBoxRequest.mutable_lower_right()->set_x( m_statsBox_i1 );
    m_statsBoxRequest.mutable_lower_right()->set_y( m_statsBox_j1 );

    m_statsBoxInflight_i0 = m_statsBox_i0;
    m_statsBoxInflight_i1 = m_statsBox_i1;
    m_statsBoxInflight_j0 = m_statsBox_j0;
    m_statsBoxInflight_j1 = m_statsBox_j1;

    m_StatsBoxContext = new grpc::ClientContext;
    m_StatsBoxContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_statsBoxPending = true;

    stub_->async()->StatsBox( m_StatsBoxContext,
                              &m_statsBoxRequest,
                              &m_statsBoxReply,
                              [this]( grpc::Status status ) { this->StatsBox_callback( status ); } );
}

void rtimvClientBase::ImageReceived()
{
    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        if( m_shuttingDown )
        {
            return;
        }
    }

    uint32_t nx, ny, nz;
    uint32_t imageNo;
    float fpsEst;
    double imageTime;
    uint32_t saturated;
    uint32_t sourceBytesPerPixel;
    float minsc, maxsc, minim, maxim;
    int imageTimeout;
    int quality;

    remote_rtimv::Colorbar colorbar;
    remote_rtimv::Colormode colormode;
    remote_rtimv::Colorstretch stretch;

    bool autoScale;
    bool subtractDark;
    bool applyMask;
    bool applySatMask;
    remote_rtimv::HPFilter hpFilter;
    float hpfFW;
    bool applyHPFilter;
    remote_rtimv::LPFilter lpFilter;
    float lpfFW;
    bool applyLPFilter;
    bool statsBox;
    uint32_t statsBox_i0, statsBox_i1, statsBox_j0, statsBox_j1;
    float statsBox_min, statsBox_max, statsBox_mean, statsBox_median;
    bool colorBox;
    uint32_t colorBox_i0, colorBox_i1, colorBox_j0, colorBox_j1;
    float colorBox_min, colorBox_max;
    int cubeDir;
    bool hasImagePayload{ false };
    size_t compressedBytes{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( m_imageRequestPending )
        {
            std::cerr << formatBaseLogMessage( std::format(
                             "bug: in ImageRecieved but imageRequestPending is true {} {}", __FILE__, __LINE__ ) )
                      << '\n';
            return;
        }

        nx = m_grpcImage.nx();
        ny = m_grpcImage.ny();
        nz = m_grpcImage.nz();
        imageNo = m_grpcImage.no();

        // Always have at least one pixel
        if( nx == 0 )
        {
            nx = 1;
        }

        if( ny == 0 )
        {
            ny = 1;
        }

        if( nz == 0 )
        {
            nz = 1;
        }

        if( !m_qim )
        {
            m_qim = new QImage;
        }

        if( m_grpcImage.image().size() > 0 )
        {
            compressedBytes = static_cast<size_t>( m_grpcImage.image().size() );
            hasImagePayload = m_qim->loadFromData(
                reinterpret_cast<const uchar *>( m_grpcImage.image().data() ), m_grpcImage.image().size(), "jpeg" );
        }

        imageTime = m_grpcImage.atime();
        fpsEst = m_grpcImage.fps();

        saturated = m_grpcImage.saturated();
        sourceBytesPerPixel = m_grpcImage.source_bytes_per_pixel();

        minim = m_grpcImage.min_image_data();
        maxim = m_grpcImage.max_image_data();
        minsc = m_grpcImage.min_scale_data();
        maxsc = m_grpcImage.max_scale_data();

        colorbar = m_grpcImage.colorbar();
        colormode = m_grpcImage.colormode();
        stretch = m_grpcImage.colorstretch();

        autoScale = m_grpcImage.autoscale();

        subtractDark = m_grpcImage.subtract_dark();
        applyMask = m_grpcImage.apply_mask();
        applySatMask = m_grpcImage.apply_sat_mask();
        hpFilter = m_grpcImage.hp_filter();
        hpfFW = m_grpcImage.hpf_fw();
        applyHPFilter = m_grpcImage.apply_hp_filter();
        lpFilter = m_grpcImage.lp_filter();
        lpfFW = m_grpcImage.lpf_fw();
        applyLPFilter = m_grpcImage.apply_lp_filter();
        statsBox = m_grpcImage.stats_box();
        statsBox_i0 = m_grpcImage.stats_box_i0();
        statsBox_i1 = m_grpcImage.stats_box_i1();
        statsBox_j0 = m_grpcImage.stats_box_j0();
        statsBox_j1 = m_grpcImage.stats_box_j1();
        statsBox_min = m_grpcImage.stats_box_min();
        statsBox_max = m_grpcImage.stats_box_max();
        statsBox_mean = m_grpcImage.stats_box_mean();
        statsBox_median = m_grpcImage.stats_box_median();
        colorBox = m_grpcImage.color_box();
        colorBox_i0 = m_grpcImage.color_box_i0();
        colorBox_i1 = m_grpcImage.color_box_i1();
        colorBox_j0 = m_grpcImage.color_box_j0();
        colorBox_j1 = m_grpcImage.color_box_j1();
        colorBox_min = m_grpcImage.color_box_min();
        colorBox_max = m_grpcImage.color_box_max();

        imageTimeout = m_grpcImage.image_timeout();
        cubeDir = m_grpcImage.cube_dir();
        quality = m_grpcImage.quality();
    }

    std::shared_mutex dummy; // Used just to satisfy the requirement, not needed
    uniqueLockT ulock( dummy );

    if( m_qim->width() != nx || m_qim->height() != ny )
    {
    }

    bool resized = false;

    m_nz = nz;
    m_foundation->emit_nzUpdated( m_nz );

    if( nx != m_nx || ny != m_ny )
    {
        m_nx = nx;
        m_ny = ny;
        mtxL_postSetImsize( ulock );

        resized = true;
    }

    m_fpsEst = fpsEst;
    updateRollingTransportStats( nx, ny, compressedBytes, sourceBytesPerPixel, hasImagePayload );
    m_imageTime = imageTime;
    m_imageNo = imageNo;
    m_saturated = saturated;
    m_minImageData = minim;
    m_maxImageData = maxim;
    m_minScaleData = minsc;
    m_maxScaleData = maxsc;

    bool sendQueuedMinScale{ false };
    float nextMinScale{ 0 };
    bool sendQueuedMaxScale{ false };
    float nextMaxScale{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );

        const auto scaleMatched = []( float a, float b )
        {
            const float norm = std::max( std::fabs( a ), std::fabs( b ) );
            const float tol = 1e-5f * std::max( 1.0f, norm );
            return std::fabs( a - b ) <= tol;
        };

        if( m_setMinScaleAwaitImage )
        {
            if( !scaleMatched( m_minScaleData, m_setMinScalePrevAtSend ) ||
                scaleMatched( m_minScaleData, m_setMinScaleInflight ) )
            {
                m_setMinScaleAwaitImage = false;
                if( m_setMinScaleQueued && !m_setMinScalePending && !m_shuttingDown )
                {
                    sendQueuedMinScale = true;
                    nextMinScale = m_setMinScaleDesired;
                    m_setMinScaleQueued = false;
                }
            }
        }

        if( m_setMaxScaleAwaitImage )
        {
            if( !scaleMatched( m_maxScaleData, m_setMaxScalePrevAtSend ) ||
                scaleMatched( m_maxScaleData, m_setMaxScaleInflight ) )
            {
                m_setMaxScaleAwaitImage = false;
                if( m_setMaxScaleQueued && !m_setMaxScalePending && !m_shuttingDown )
                {
                    sendQueuedMaxScale = true;
                    nextMaxScale = m_setMaxScaleDesired;
                    m_setMaxScaleQueued = false;
                }
            }
        }
    }

    m_colorbar = rtimv::grpc2colorbar( colorbar );
    m_colormode = rtimv::grpc2colormode( colormode );
    m_stretch = rtimv::grpc2stretch( stretch );

    m_autoScale = autoScale;

    m_subtractDark = subtractDark;
    m_applyMask = applyMask;
    m_applySatMask = applySatMask;
    rtimv::hpFilter hp = rtimv::grpc2hpFilter( hpFilter );
    if( hp != static_cast<rtimv::hpFilter>( -1 ) )
    {
        m_hpFilter = hp;
    }
    m_hpfFW = hpfFW;
    m_applyHPFilter = applyHPFilter;
    rtimv::lpFilter lp = rtimv::grpc2lpFilter( lpFilter );
    if( lp != static_cast<rtimv::lpFilter>( -1 ) )
    {
        m_lpFilter = lp;
    }
    m_lpfFW = lpfFW;
    m_applyLPFilter = applyLPFilter;
    m_statsBox = statsBox;
    m_statsBox_i0 = statsBox_i0;
    m_statsBox_i1 = statsBox_i1;
    m_statsBox_j0 = statsBox_j0;
    m_statsBox_j1 = statsBox_j1;
    m_statsBox_min = statsBox_min;
    m_statsBox_max = statsBox_max;
    m_statsBox_mean = statsBox_mean;
    m_statsBox_median = statsBox_median;
    m_colorBox_i0 = colorBox_i0;
    m_colorBox_i1 = colorBox_i1;
    m_colorBox_j0 = colorBox_j0;
    m_colorBox_j1 = colorBox_j1;
    m_colorBox_min = colorBox_min;
    m_colorBox_max = colorBox_max;
    m_imageTimeout = imageTimeout;
    m_quality = quality;
    if( m_cubeDir != cubeDir )
    {
        m_cubeDir = cubeDir;
        m_foundation->emit_cubeDirUpdated( m_cubeDir );
    }

    m_foundation->emit_statsBoxUpdated( m_statsBox_i0,
                                        m_statsBox_i1,
                                        m_statsBox_j0,
                                        m_statsBox_j1,
                                        m_statsBox_min,
                                        m_statsBox_max,
                                        m_statsBox_mean,
                                        m_statsBox_median,
                                        m_statsBox );

    m_foundation->emit_colorBoxUpdated(
        m_colorBox_i0, m_colorBox_i1, m_colorBox_j0, m_colorBox_j1, m_colorBox_min, m_colorBox_max, colorBox );

    if( hasImagePayload )
    {
        mtxL_postRecolor( ulock );

        ulock.unlock();
        sharedLockT slock( dummy );
        mtxL_postChangeImdata( slock );
    }
    else
    {
        ulock.unlock();
    }

    if( resized && hasImagePayload )
    {
        // Always switch to zoom 1 after a resize occurs
        zoomLevel( 1 );
    }

    updateFPS();

    if( sendQueuedMinScale )
    {
        minScaleData( nextMinScale );
    }

    if( sendQueuedMaxScale )
    {
        maxScaleData( nextMaxScale );
    }

    // We always go on and get the next one
    m_foundation->emit_ImageNeeded();
}

void rtimvClientBase::ImagePlease_callback( grpc::Status status )
{
    int action = -1;
    int retryMs = 0;
    bool shuttingDown = false;
    bool disconnected = false;
    uint64_t connections = 0;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( !m_imageRequestPending )
        {
            std::cerr << formatBaseLogMessage( "bug: in ImagePlease_callback but imageRequestPending is false" )
                      << '\n';
            return;
        }

        // Act upon its status.
        if( status.ok() )
        {
            if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_VALID )
            {
                action = 0;
                m_imageRetryBackoffMs = 500;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_NO_IMAGE )
            {
                action = 1;
                m_imageRetryBackoffMs = std::min( 5000, std::max( 1000, m_imageRetryBackoffMs ) * 2 );
                retryMs = m_imageRetryBackoffMs;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_TIMEOUT )
            {
                action = 3;
                m_imageRetryBackoffMs = std::min( 5000, std::max( 500, m_imageRetryBackoffMs ) * 2 );
                retryMs = m_imageRetryBackoffMs;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_NOT_CONFIGURED )
            {
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_ERROR )
            {
            }
        }
        else
        {
            if( status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED )
            {
                // Under heavy server load this can be transient; continue polling without forcing reconnect.
                action = 2;
            }
            else
            {
                disconnected = true;
            }
        }

        delete m_ImagePleaseContext;
        m_ImagePleaseContext = nullptr;

        m_imageRequestPending = false;
        shuttingDown = m_shuttingDown;
    }

    m_imageRequestCv.notify_all();

    if( disconnected )
    {
        sharedLockT connLock( m_connectedMutex );
        connections = m_connections;
        connLock.unlock();

        uniqueLockT lock( m_connectedMutex );
        if( connections == m_connections )
        {
            m_connected = false;
            std::cerr << formatBaseLogMessage( std::string( "Message from rtimvServer: " ) + status.error_message() )
                      << '\n';
        }
    }

    if( shuttingDown )
    {
        return;
    }

    auto scheduleImageRetry = [this]( int ms )
    {
        QPointer<rtimvBaseObject> foundation = m_foundation;
        std::thread(
            [foundation, ms]()
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
                if( foundation )
                {
                    QMetaObject::invokeMethod( foundation, "ImagePlease", Qt::QueuedConnection );
                }
            } )
            .detach();
    };

    if( action == 0 )
    {
        m_foundation->emit_ImageWaiting();
    }
    else if( action == 1 )
    {
        // Avoid a tight no-image polling loop.
        scheduleImageRetry( retryMs > 0 ? retryMs : 1000 );
    }
    else if( action == 2 )
    {
        // Deadline expired waiting on the server; retry without disconnect/reconfigure churn.
        m_foundation->emit_ImageNeeded();
    }
    else if( action == 3 )
    {
        // For no-new-frame timeouts, avoid re-processing stale image state for every client.
        scheduleImageRetry( retryMs > 0 ? retryMs : 500 );
    }

    // if action stays -1 we just return
}

void rtimvClientBase::GetPixel_callback( grpc::Status status )
{
    bool queueNext{ false };
    uint32_t nextX{ 0 };
    uint32_t nextY{ 0 };
    uint32_t reqX{ 0 };
    uint32_t reqY{ 0 };
    float value{ 0 };
    bool valid{ false };

    { // mutex scope
        std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );

        if( !m_getPixelPending )
        {
            std::cerr << formatBaseLogMessage( "bug: in GetPixel_callback but getPixelPending is false" ) << '\n';
            return;
        }

        reqX = m_getPixelInflightX;
        reqY = m_getPixelInflightY;

        if( status.ok() )
        {
            valid = m_getPixelReply.valid();
            if( valid )
            {
                value = m_getPixelReply.value();
            }
        }
        else
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
        }

        delete m_GetPixelContext;
        m_GetPixelContext = nullptr;

        m_getPixelPending = false;
        queueNext = m_getPixelQueued && !m_shuttingDown;
        nextX = m_getPixelX;
        nextY = m_getPixelY;
        m_getPixelQueued = false;
    }

    m_asyncRpcCv.notify_all();

    if( m_foundation )
    {
        m_foundation->emit_pixelValueUpdated( reqX, reqY, value, valid );
    }

    if( queueNext )
    {
        requestPixelValue( nextX, nextY );
    }
}

void rtimvClientBase::ColorBox_callback( grpc::Status status )
{
    bool queueNext{ false };
    bool valid{ false };
    int64_t i0{ 0 }, i1{ 0 }, j0{ 0 }, j1{ 0 };
    float min{ 0 }, max{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );

        if( !m_colorBoxPending )
        {
            std::cerr << formatBaseLogMessage( "bug: in ColorBox_callback but colorBoxPending is false" ) << '\n';
            return;
        }

        i0 = m_colorBoxInflight_i0;
        i1 = m_colorBoxInflight_i1;
        j0 = m_colorBoxInflight_j0;
        j1 = m_colorBoxInflight_j1;

        if( status.ok() )
        {
            valid = m_colorBoxReply.valid();
            if( valid )
            {
                min = m_colorBoxReply.min();
                max = m_colorBoxReply.max();

                m_colorBox_i0 = i0;
                m_colorBox_i1 = i1;
                m_colorBox_j0 = j0;
                m_colorBox_j1 = j1;
                m_colorBox_min = min;
                m_colorBox_max = max;
            }
        }
        else
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
        }

        delete m_ColorBoxContext;
        m_ColorBoxContext = nullptr;

        m_colorBoxPending = false;
        queueNext = m_colorBoxQueued && !m_shuttingDown;
        m_colorBoxQueued = false;
    }

    m_asyncRpcCv.notify_all();

    if( m_foundation )
    {
        m_foundation->emit_colorBoxUpdated( i0, i1, j0, j1, min, max, valid );
    }

    if( queueNext )
    {
        requestColorBoxValues();
    }
}

void rtimvClientBase::StatsBox_callback( grpc::Status status )
{
    bool queueNext{ false };
    bool valid{ false };
    int64_t i0{ 0 }, i1{ 0 }, j0{ 0 }, j1{ 0 };
    float min{ 0 }, max{ 0 }, mean{ 0 }, median{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

        if( !m_statsBoxPending )
        {
            std::cerr << formatBaseLogMessage( "bug: in StatsBox_callback but statsBoxPending is false" ) << '\n';
            return;
        }

        i0 = m_statsBoxInflight_i0;
        i1 = m_statsBoxInflight_i1;
        j0 = m_statsBoxInflight_j0;
        j1 = m_statsBoxInflight_j1;

        if( status.ok() )
        {
            valid = m_statsBoxReply.valid();
            if( valid )
            {
                i0 = m_statsBoxReply.i0();
                i1 = m_statsBoxReply.i1();
                j0 = m_statsBoxReply.j0();
                j1 = m_statsBoxReply.j1();
                min = m_statsBoxReply.min();
                max = m_statsBoxReply.max();
                mean = m_statsBoxReply.mean();
                median = m_statsBoxReply.median();

                m_statsBox = true;
                m_statsBox_i0 = i0;
                m_statsBox_i1 = i1;
                m_statsBox_j0 = j0;
                m_statsBox_j1 = j1;
                m_statsBox_min = min;
                m_statsBox_max = max;
                m_statsBox_mean = mean;
                m_statsBox_median = median;
            }
        }
        else
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
        }

        delete m_StatsBoxContext;
        m_StatsBoxContext = nullptr;

        m_statsBoxPending = false;
        queueNext = m_statsBoxQueued && !m_shuttingDown;
        m_statsBoxQueued = false;
    }

    m_asyncRpcCv.notify_all();

    if( m_foundation )
    {
        m_foundation->emit_statsBoxUpdated( i0, i1, j0, j1, min, max, mean, median, valid );
    }

    if( queueNext )
    {
        requestStatsBoxValues();
    }
}

void rtimvClientBase::SetColorstretch_callback( grpc::Status status )
{
    bool queueNext{ false };
    rtimv::stretch nextStretch{ rtimv::stretch::linear };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

        if( !m_setColorstretchPending )
        {
            std::cerr << formatBaseLogMessage( "bug: in SetColorstretch_callback but setColorstretchPending is false" )
                      << '\n';
            return;
        }

        if( !status.ok() )
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
        }

        delete m_SetColorstretchContext;
        m_SetColorstretchContext = nullptr;

        m_setColorstretchPending = false;
        queueNext = m_setColorstretchQueued && !m_shuttingDown;
        nextStretch = m_setColorstretchDesired;
        m_setColorstretchQueued = false;
    }

    m_asyncRpcCv.notify_all();

    if( queueNext )
    {
        stretch( nextStretch );
    }
}

void rtimvClientBase::SetMinScale_callback( grpc::Status status )
{
    bool queueNext{ false };
    float nextValue{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

        if( !m_setMinScalePending )
        {
            std::cerr << formatBaseLogMessage( "bug: in SetMinScale_callback but setMinScalePending is false" ) << '\n';
            return;
        }

        if( status.ok() )
        {
            m_setMinScaleAwaitImage = true;
        }
        else
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
            queueNext = m_setMinScaleQueued && !m_shuttingDown;
            nextValue = m_setMinScaleDesired;
            m_setMinScaleQueued = false;
        }

        delete m_SetMinScaleContext;
        m_SetMinScaleContext = nullptr;
        m_setMinScalePending = false;
    }

    m_asyncRpcCv.notify_all();

    if( queueNext )
    {
        minScaleData( nextValue );
    }
}

void rtimvClientBase::SetMaxScale_callback( grpc::Status status )
{
    bool queueNext{ false };
    float nextValue{ 0 };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

        if( !m_setMaxScalePending )
        {
            std::cerr << formatBaseLogMessage( "bug: in SetMaxScale_callback but setMaxScalePending is false" ) << '\n';
            return;
        }

        if( status.ok() )
        {
            m_setMaxScaleAwaitImage = true;
        }
        else
        {
            sharedLockT connLock( m_connectedMutex );
            REPORT_SERVER_DISCONNECTED
            queueNext = m_setMaxScaleQueued && !m_shuttingDown;
            nextValue = m_setMaxScaleDesired;
            m_setMaxScaleQueued = false;
        }

        delete m_SetMaxScaleContext;
        m_SetMaxScaleContext = nullptr;
        m_setMaxScalePending = false;
    }

    m_asyncRpcCv.notify_all();

    if( queueNext )
    {
        maxScaleData( nextValue );
    }
}

void rtimvClientBase::EmptyRpc_callback( grpc::ClientContext *context, grpc::Status status )
{
    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

        auto it = std::find( m_emptyRpcContexts.begin(), m_emptyRpcContexts.end(), context );
        if( it == m_emptyRpcContexts.end() )
        {
            std::cerr << formatBaseLogMessage( "bug: in EmptyRpc_callback but context is not tracked" ) << '\n';
        }
        else
        {
            m_emptyRpcContexts.erase( it );
        }

        if( m_emptyRpcPending == 0 )
        {
            std::cerr << formatBaseLogMessage( "bug: in EmptyRpc_callback but emptyRpcPending is zero" ) << '\n';
        }
        else
        {
            --m_emptyRpcPending;
        }
    }

    delete context;

    m_asyncRpcCv.notify_all();

    if( !status.ok() )
    {
        sharedLockT connLock( m_connectedMutex );
        REPORT_SERVER_DISCONNECTED
    }
}

bool rtimvClientBase::imageValid()
{
    return true;
}

bool rtimvClientBase::imageValid( size_t n )
{
    return true;
}

double rtimvClientBase::imageTime()
{
    return m_imageTime;
}

double rtimvClientBase::imageTime( size_t n )
{
    if( n == 0 )
    {
        return m_imageTime;
    }

    // get from server?
    return 0;
}

double rtimvClientBase::fpsEst()
{
    return m_fpsEst;
}

double rtimvClientBase::fpsEst( size_t n )
{
    if( n == 0 )
    {
        return m_fpsEst;
    }

    // get from server?
    return 0;
}

double rtimvClientBase::lastCompressionRatio()
{
    return m_lastCompressionRatio;
}

double rtimvClientBase::avgCompressionRatio()
{
    return m_avgCompressionRatio;
}

double rtimvClientBase::avgFrameRate()
{
    return m_avgFrameRate;
}

std::string rtimvClientBase::imageName( size_t n )
{
    if( n >= m_imageNames.size() )
    {
        return "";
    }

    return m_imageNames[n];
}

uint32_t rtimvClientBase::imageNo( size_t n )
{
    if( n == 0 )
    {
        return m_imageNo;
    }

    SHARED_CONN_LOCK_RET( 0 )

    remote_rtimv::ImageNoRequest request;
    remote_rtimv::ImageNoResponse response;
    grpc::ClientContext context;

    request.set_image( static_cast<uint32_t>( n ) );

    grpc::Status status = stub_->GetImageNo( &context, request, &response );

    if( status.ok() )
    {
        if( !response.valid() )
        {
            return 0;
        }

        return response.no();
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return 0;
    }
}

std::vector<std::string> rtimvClientBase::info( size_t n )
{
    SHARED_CONN_LOCK_RET( std::vector<std::string>( { "" } ) )

    remote_rtimv::InfoRequest request;
    remote_rtimv::InfoResponse response;
    grpc::ClientContext context;

    request.set_image( static_cast<uint32_t>( n ) );

    grpc::Status status = stub_->GetInfo( &context, request, &response );

    if( status.ok() )
    {
        if( !response.valid() )
        {
            return std::vector<std::string>( { "" } );
        }

        std::vector<std::string> info;
        info.reserve( response.info_size() );
        for( const auto &s : response.info() )
        {
            info.push_back( s );
        }

        if( info.size() == 0 )
        {
            return std::vector<std::string>( { "" } );
        }

        return info;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return std::vector<std::string>( { "" } );
    }
}

void rtimvClientBase::mtxL_setImsize( uint32_t x, uint32_t y, uint32_t z, const uniqueLockT &lock )
{

    m_nz = z;
    m_foundation->emit_nzUpdated( m_nz );

    if( m_nx != x || m_ny != y )
    {
        m_nx = x;
        m_ny = y;

        mtxL_postSetImsize( lock );
    }
}

uint32_t rtimvClientBase::nx()
{
    return m_nx;
}

uint32_t rtimvClientBase::ny()
{
    return m_ny;
}

uint32_t rtimvClientBase::nz()
{
    return m_nz;
}

void rtimvClientBase::cubeMode( bool cm )
{
    m_cubeMode = cm;

    setCurrImageTimeout();

    m_foundation->emit_cubeModeUpdated( m_cubeMode );
}

void rtimvClientBase::cubeFPS( float fps )
{
    if( fps < 0 )
    {
        fps = 0;
    }
    m_desiredCubeFPS = fps;
    setCurrImageTimeout();

    m_foundation->emit_cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
}

void rtimvClientBase::cubeFPSMult( float mult )
{
    m_cubeFPSMult = mult;
    setCurrImageTimeout();
    m_foundation->emit_cubeFPSMultUpdated( m_cubeFPSMult );
}

void rtimvClientBase::cubeDir( int dir )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::CubeDirRequest>();
    auto response = std::make_shared<remote_rtimv::CubeDirResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_dir( dir );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->CubeDir( context,
                             request.get(),
                             response.get(),
                             [this, context, request, response]( grpc::Status status )
                             { this->EmptyRpc_callback( context, status ); } );
}

void rtimvClientBase::cubeFrame( uint32_t fno )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::CubeFrameRequest>();
    auto response = std::make_shared<remote_rtimv::CubeFrameResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_frame( fno );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->CubeFrame( context,
                               request.get(),
                               response.get(),
                               [this, context, request, response]( grpc::Status status )
                               { this->EmptyRpc_callback( context, status ); } );
}

void rtimvClientBase::cubeFrameDelta( int32_t dfno )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::CubeFrameDeltaRequest>();
    auto response = std::make_shared<remote_rtimv::CubeFrameDeltaResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_delta( dfno );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->CubeFrameDelta( context,
                                    request.get(),
                                    response.get(),
                                    [this, context, request, response]( grpc::Status status )
                                    { this->EmptyRpc_callback( context, status ); } );
}

void rtimvClientBase::updateCube()
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::UpdateCubeRequest>();
    auto response = std::make_shared<remote_rtimv::UpdateCubeResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->UpdateCube( context,
                                request.get(),
                                response.get(),
                                [this, context, request, response]( grpc::Status status )
                                { this->EmptyRpc_callback( context, status ); } );
}

void rtimvClientBase::updateCubeFrame()
{
    m_foundation->emit_cubeFrameUpdated( imageNo( 0 ) );
}

void rtimvClientBase::setCurrImageTimeout()
{
    int cubeTimeout;

    if( m_desiredCubeFPS <= 0 || m_nz <= 1 )
    {
        m_foundation->m_cubeTimer.stop();

        if( m_nz <= 1 )
        {
            m_foundation->m_cubeFrameUpdateTimer.stop();
        }
        else
        {
            m_foundation->m_cubeFrameUpdateTimer.start( 250 );
        }

        m_cubeFPS = 0;

        m_foundation->emit_cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
    }
    else // it's a cube, cube mode is on, and FPS > 0
    {
        // First get our wish
        cubeTimeout = std::round( 1000. / ( m_desiredCubeFPS * m_cubeFPSMult ) );

        if( cubeTimeout < 1 )
        {
            cubeTimeout = 1;
        }

        if( cubeTimeout < m_imageTimeout )
        {
            cubeTimeout = m_imageTimeout;
        }

        // Now get reality with imageTimeout
        int f = std::round( ( 1.0 * cubeTimeout ) / m_imageTimeout );
        if( f <= 0 )
        {
            f = 1;
        }

        // Report reality
        m_cubeFPS = ( 1000.0 / ( f * m_imageTimeout ) ) / m_cubeFPSMult;

        // Implement reality
        cubeTimeout = std::round( 1000. / ( m_cubeFPS * m_cubeFPSMult ) );

        if( m_cubeMode )
        {
            m_foundation->m_cubeTimer.start( cubeTimeout );
            m_foundation->m_cubeFrameUpdateTimer.start( 250 );
        }
        else
        {
            m_cubeFPS = 0;
            m_foundation->m_cubeTimer.stop();
        }

        m_foundation->emit_cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
    }
}

void rtimvClientBase::updateRollingTransportStats(
    uint32_t nx, uint32_t ny, size_t compressedBytes, uint32_t sourceBytesPerPixel, bool validPayload )
{
    if( m_rollingStatsFrames < 1 )
    {
        m_rollingStatsFrames = 1;
    }

    const auto now = std::chrono::steady_clock::now();
    if( m_lastArrivalTime != std::chrono::steady_clock::time_point{} )
    {
        const std::chrono::duration<double> dt = now - m_lastArrivalTime;
        const double seconds = dt.count();
        if( seconds > 0 )
        {
            m_recentFrameIntervals.push_back( seconds );
            m_fpsRollingSum += seconds;

            while( static_cast<int>( m_recentFrameIntervals.size() ) > m_rollingStatsFrames )
            {
                m_fpsRollingSum -= m_recentFrameIntervals.front();
                m_recentFrameIntervals.pop_front();
            }

            if( !m_recentFrameIntervals.empty() )
            {
                const double avgFrameInterval = m_fpsRollingSum / static_cast<double>( m_recentFrameIntervals.size() );
                if( avgFrameInterval > 0 )
                {
                    m_avgFrameRate = 1.0 / avgFrameInterval;
                }
            }
        }
    }
    m_lastArrivalTime = now;

    if( !validPayload || compressedBytes == 0 || sourceBytesPerPixel == 0 )
    {
        return;
    }

    const double sourceBytes =
        static_cast<double>( nx ) * static_cast<double>( ny ) * static_cast<double>( sourceBytesPerPixel );
    if( sourceBytes <= 0 )
    {
        return;
    }

    const double compressionRatio = sourceBytes / static_cast<double>( compressedBytes );
    m_lastCompressionRatio = compressionRatio;

    m_recentCompressionRatios.push_back( compressionRatio );
    m_compressionRollingSum += compressionRatio;

    while( static_cast<int>( m_recentCompressionRatios.size() ) > m_rollingStatsFrames )
    {
        m_compressionRollingSum -= m_recentCompressionRatios.front();
        m_recentCompressionRatios.pop_front();
    }

    if( !m_recentCompressionRatios.empty() )
    {
        m_avgCompressionRatio = m_compressionRollingSum / static_cast<double>( m_recentCompressionRatios.size() );
    }
}

void rtimvClientBase::imageTimeout( int to )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ImageTimeoutRequest>();
    auto response = std::make_shared<remote_rtimv::ImageTimeoutResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_timeout( to );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetImageTimeout( context,
                                     request.get(),
                                     response.get(),
                                     [this, context, request, response]( grpc::Status status )
                                     { this->EmptyRpc_callback( context, status ); } );
}

int rtimvClientBase::imageTimeout()
{
    return m_imageTimeout;
}

void rtimvClientBase::quality( int q )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::QualityRequest>();
    auto response = std::make_shared<remote_rtimv::QualityResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_quality( q );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetQuality( context,
                                request.get(),
                                response.get(),
                                [this, context, request, response]( grpc::Status status )
                                { this->EmptyRpc_callback( context, status ); } );
}

int rtimvClientBase::quality()
{
    return m_quality;
}

void rtimvClientBase::subtractDark( bool sd )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::SubDarkRequest>();
    auto response = std::make_shared<remote_rtimv::SubDarkResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_subtract_dark( sd );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetSubDark( context,
                                request.get(),
                                response.get(),
                                [this, context, request, response]( grpc::Status status )
                                { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::subtractDark()
{
    return m_subtractDark;
}

void rtimvClientBase::applyMask( bool amsk )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ApplyMaskRequest>();
    auto response = std::make_shared<remote_rtimv::ApplyMaskResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_apply_mask( amsk );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetApplyMask( context,
                                  request.get(),
                                  response.get(),
                                  [this, context, request, response]( grpc::Status status )
                                  { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::applyMask()
{
    return m_applyMask;
}

void rtimvClientBase::applySatMask( bool asmsk )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ApplySatMaskRequest>();
    auto response = std::make_shared<remote_rtimv::ApplySatMaskResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_apply_sat_mask( asmsk );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetApplySatMask( context,
                                     request.get(),
                                     response.get(),
                                     [this, context, request, response]( grpc::Status status )
                                     { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::applySatMask()
{
    return m_applySatMask;
}

void rtimvClientBase::hpFilter( rtimv::hpFilter filter )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::HPFilterRequest>();
    auto response = std::make_shared<remote_rtimv::HPFilterResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_hp_filter( rtimv::hpFilter2grpc( filter ) );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetHPFilter( context,
                                 request.get(),
                                 response.get(),
                                 [this, context, request, response]( grpc::Status status )
                                 { this->EmptyRpc_callback( context, status ); } );
}

rtimv::hpFilter rtimvClientBase::hpFilter()
{
    return m_hpFilter;
}

void rtimvClientBase::hpfFW( float fw )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::FilterWidthRequest>();
    auto response = std::make_shared<remote_rtimv::FilterWidthResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_width( fw );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetHPFW( context,
                             request.get(),
                             response.get(),
                             [this, context, request, response]( grpc::Status status )
                             { this->EmptyRpc_callback( context, status ); } );
}

float rtimvClientBase::hpfFW()
{
    return m_hpfFW;
}

void rtimvClientBase::applyHPFilter( bool apply )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ApplyFilterRequest>();
    auto response = std::make_shared<remote_rtimv::ApplyFilterResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_apply_filter( apply );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetApplyHPFilter( context,
                                      request.get(),
                                      response.get(),
                                      [this, context, request, response]( grpc::Status status )
                                      { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::applyHPFilter()
{
    return m_applyHPFilter;
}

void rtimvClientBase::lpFilter( rtimv::lpFilter filter )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::LPFilterRequest>();
    auto response = std::make_shared<remote_rtimv::LPFilterResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_lp_filter( rtimv::lpFilter2grpc( filter ) );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetLPFilter( context,
                                 request.get(),
                                 response.get(),
                                 [this, context, request, response]( grpc::Status status )
                                 { this->EmptyRpc_callback( context, status ); } );
}

rtimv::lpFilter rtimvClientBase::lpFilter()
{
    return m_lpFilter;
}

void rtimvClientBase::lpfFW( float fw )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::FilterWidthRequest>();
    auto response = std::make_shared<remote_rtimv::FilterWidthResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_width( fw );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetLPFW( context,
                             request.get(),
                             response.get(),
                             [this, context, request, response]( grpc::Status status )
                             { this->EmptyRpc_callback( context, status ); } );
}

float rtimvClientBase::lpfFW()
{
    return m_lpfFW;
}

void rtimvClientBase::applyLPFilter( bool apply )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ApplyFilterRequest>();
    auto response = std::make_shared<remote_rtimv::ApplyFilterResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_apply_filter( apply );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetApplyLPFilter( context,
                                      request.get(),
                                      response.get(),
                                      [this, context, request, response]( grpc::Status status )
                                      { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::applyLPFilter()
{
    return m_applyLPFilter;
}

float rtimvClientBase::calPixel( uint32_t x, uint32_t y )
{
    requestPixelValue( x, y );
    return 0;
}

void rtimvClientBase::mtxL_load_colorbarImpl( rtimv::colorbar cb, bool update )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::ColorbarRequest>();
    auto response = std::make_shared<remote_rtimv::ColorbarResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_colorbar( rtimv::colorbar2grpc( cb ) );
    request->set_update( update );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetColorbar( context,
                                 request.get(),
                                 response.get(),
                                 [this, context, request, response]( grpc::Status status )
                                 { this->EmptyRpc_callback( context, status ); } );
}

void rtimvClientBase::mtxL_load_colorbar( rtimv::colorbar cb, bool update, const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_load_colorbarImpl( cb, update );
}

void rtimvClientBase::mtxL_load_colorbar( rtimv::colorbar cb, bool update, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_load_colorbarImpl( cb, update );
}

void rtimvClientBase::mtxUL_load_colorbar( rtimv::colorbar cb, bool update )
{
    sharedLockT lock( m_calMutex );

    mtxL_load_colorbar( cb, update, lock );
}

rtimv::colorbar rtimvClientBase::colorbar()
{
    return m_colorbar;
}

void rtimvClientBase::mtxUL_colormode( rtimv::colormode m )
{
    sharedLockT lock( m_calMutex );

    mtxL_colormode( m, lock );
}

void rtimvClientBase::mtxL_colormode( rtimv::colormode m, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    if( m == rtimv::colormode::minmaxbox )
    {
        requestColorBoxValues();
    }
    else
    {
        SHARED_CONN_LOCK

        auto request = std::make_shared<remote_rtimv::ColormodeRequest>();
        auto response = std::make_shared<remote_rtimv::ColormodeResponse>();
        auto *context = new grpc::ClientContext;

        { // mutex scope
            std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );
            if( m_shuttingDown )
            {
                delete context;
                return;
            }
            m_emptyRpcContexts.push_back( context );
            ++m_emptyRpcPending;
        }

        request->set_colormode( rtimv::colormode2grpc( m ) );

        context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
        stub_->async()->SetColormode( context,
                                      request.get(),
                                      response.get(),
                                      [this, context, request, response]( grpc::Status status )
                                      { this->EmptyRpc_callback( context, status ); } );
    }

    m_colormode = m;

    mtxL_postColormode( m, lock );
}

rtimv::colormode rtimvClientBase::colormode()
{
    return m_colormode;
}

void rtimvClientBase::colorBox_i0( int64_t i0 )
{
    m_colorBox_i0 = i0;
}

int64_t rtimvClientBase::colorBox_i0()
{
    return m_colorBox_i0;
}

void rtimvClientBase::colorBox_i1( int64_t i1 )
{
    m_colorBox_i1 = i1;
}

int64_t rtimvClientBase::colorBox_i1()
{
    return m_colorBox_i1;
}

void rtimvClientBase::colorBox_j0( int64_t j0 )
{
    m_colorBox_j0 = j0;
}

int64_t rtimvClientBase::colorBox_j0()
{
    return m_colorBox_j0;
}

void rtimvClientBase::colorBox_j1( int64_t j1 )
{
    m_colorBox_j1 = j1;
}

int64_t rtimvClientBase::colorBox_j1()
{
    return m_colorBox_j1;
}

float rtimvClientBase::colorBox_min()
{
    return m_colorBox_min;
}

float rtimvClientBase::colorBox_max()
{
    return m_colorBox_max;
}

void rtimvClientBase::statsBox( bool sb )
{
    m_statsBox = sb;

    if( sb )
    {
        return;
    }

    m_statsBox_min = 0;
    m_statsBox_max = 0;
    m_statsBox_mean = 0;
    m_statsBox_median = 0;
    m_statsBox_i0 = 1;
    m_statsBox_j0 = 1;
    m_statsBox_i1 = 0;
    m_statsBox_j1 = 0;

    requestStatsBoxValues();
}

bool rtimvClientBase::statsBox()
{
    return m_statsBox;
}

void rtimvClientBase::statsBox_i0( int64_t i0 )
{
    m_statsBox_i0 = i0;
}

int64_t rtimvClientBase::statsBox_i0()
{
    return m_statsBox_i0;
}

void rtimvClientBase::statsBox_i1( int64_t i1 )
{
    m_statsBox_i1 = i1;
}

int64_t rtimvClientBase::statsBox_i1()
{
    return m_statsBox_i1;
}

void rtimvClientBase::statsBox_j0( int64_t j0 )
{
    m_statsBox_j0 = j0;
}

int64_t rtimvClientBase::statsBox_j0()
{
    return m_statsBox_j0;
}

void rtimvClientBase::statsBox_j1( int64_t j1 )
{
    m_statsBox_j1 = j1;
}

int64_t rtimvClientBase::statsBox_j1()
{
    return m_statsBox_j1;
}

float rtimvClientBase::statsBox_min()
{
    return m_statsBox_min;
}

float rtimvClientBase::statsBox_max()
{
    return m_statsBox_max;
}

float rtimvClientBase::statsBox_mean()
{
    return m_statsBox_mean;
}

float rtimvClientBase::statsBox_median()
{
    return m_statsBox_median;
}

void rtimvClientBase::mtxUL_calcStatsBox()
{
    requestStatsBoxValues();
}

void rtimvClientBase::stretch( rtimv::stretch cs )
{
    SHARED_CONN_LOCK

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    m_setColorstretchDesired = cs;

    if( m_setColorstretchPending )
    {
        m_setColorstretchQueued = true;
        return;
    }

    if( m_SetColorstretchContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in stretch but SetColorstretchContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_setColorstretchRequest.set_colorstretch( rtimv::stretch2grpc( m_setColorstretchDesired ) );

    m_SetColorstretchContext = new grpc::ClientContext;
    m_SetColorstretchContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_setColorstretchPending = true;

    stub_->async()->SetColorstretch( m_SetColorstretchContext,
                                     &m_setColorstretchRequest,
                                     &m_setColorstretchReply,
                                     [this]( grpc::Status status ) { this->SetColorstretch_callback( status ); } );
}

rtimv::stretch rtimvClientBase::stretch()
{
    return m_stretch;
}

void rtimvClientBase::minScaleData( float md )
{
    SHARED_CONN_LOCK

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    m_setMinScaleDesired = md;

    if( m_setMinScalePending || m_setMinScaleAwaitImage )
    {
        m_setMinScaleQueued = true;
        return;
    }

    if( m_SetMinScaleContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in minScaleData but SetMinScaleContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_setMinScaleRequest.set_value( m_setMinScaleDesired );
    m_setMinScaleInflight = m_setMinScaleDesired;
    m_setMinScalePrevAtSend = m_minScaleData;

    m_SetMinScaleContext = new grpc::ClientContext;
    m_SetMinScaleContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_setMinScalePending = true;

    stub_->async()->SetMinScale( m_SetMinScaleContext,
                                 &m_setMinScaleRequest,
                                 &m_setMinScaleReply,
                                 [this]( grpc::Status status ) { this->SetMinScale_callback( status ); } );
}

float rtimvClientBase::minScaleData()
{
    return m_minScaleData;
}

void rtimvClientBase::maxScaleData( float md )
{
    SHARED_CONN_LOCK

    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );

    if( m_shuttingDown )
    {
        return;
    }

    m_setMaxScaleDesired = md;

    if( m_setMaxScalePending || m_setMaxScaleAwaitImage )
    {
        m_setMaxScaleQueued = true;
        return;
    }

    if( m_SetMaxScaleContext )
    {
        std::cerr << formatBaseLogMessage( std::format(
                         "bug: in maxScaleData but SetMaxScaleContext is allocated {} {}", __FILE__, __LINE__ ) )
                  << '\n';
        return;
    }

    m_setMaxScaleRequest.set_value( m_setMaxScaleDesired );
    m_setMaxScaleInflight = m_setMaxScaleDesired;
    m_setMaxScalePrevAtSend = m_maxScaleData;

    m_SetMaxScaleContext = new grpc::ClientContext;
    m_SetMaxScaleContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    m_setMaxScalePending = true;

    stub_->async()->SetMaxScale( m_SetMaxScaleContext,
                                 &m_setMaxScaleRequest,
                                 &m_setMaxScaleReply,
                                 [this]( grpc::Status status ) { this->SetMaxScale_callback( status ); } );
}

float rtimvClientBase::maxScaleData()
{
    return m_maxScaleData;
}

void rtimvClientBase::bias( float b )
{
    float cont = contrast();

    minScaleData( b - 0.5 * cont );
    maxScaleData( b + 0.5 * cont );
}

float rtimvClientBase::bias()
{
    return 0.5 * ( m_maxScaleData + m_minScaleData );
}

void rtimvClientBase::bias_rel( float br )
{
    float cont = contrast();

    minScaleData( m_minImageData + br * ( m_maxImageData - m_minImageData ) - 0.5 * cont );
    maxScaleData( m_minImageData + br * ( m_maxImageData - m_minImageData ) + 0.5 * cont );
}

float rtimvClientBase::bias_rel()
{
    return 0.5 * ( m_maxScaleData + m_minScaleData ) / ( m_maxScaleData - m_minScaleData );
}

void rtimvClientBase::contrast( float c )
{
    float b = bias();
    minScaleData( b - 0.5 * c );
    maxScaleData( b + 0.5 * c );
}

float rtimvClientBase::contrast()
{
    return m_maxScaleData - m_minScaleData;
}

float rtimvClientBase::contrast_rel()
{
    return ( m_maxImageData - m_minImageData ) / ( m_maxScaleData - m_minScaleData );
}

void rtimvClientBase::contrast_rel( float cr )
{
    float b = bias();
    minScaleData( b - .5 * ( m_maxImageData - m_minImageData ) / cr );
    maxScaleData( b + .5 * ( m_maxImageData - m_minImageData ) / cr );
}

void rtimvClientBase::mtxUL_autoScale( bool as )
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::AutoscaleRequest>();
    auto response = std::make_shared<remote_rtimv::AutoscaleResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    request->set_autoscale( as );

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->SetAutoscale( context,
                                  request.get(),
                                  response.get(),
                                  [this, context, request, response]( grpc::Status status )
                                  { this->EmptyRpc_callback( context, status ); } );
}

bool rtimvClientBase::autoScale()
{
    return m_autoScale;
}

void rtimvClientBase::mtxUL_reStretch()
{
    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::RestretchRequest>();
    auto response = std::make_shared<remote_rtimv::RestretchResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->Restretch( context,
                               request.get(),
                               response.get(),
                               [this, context, request, response]( grpc::Status status )
                               { this->EmptyRpc_callback( context, status ); } );
}

uint8_t rtimvClientBase::lightness( int x, int y )
{
    if( !m_qim )
    {
        return 0;
    }

    return m_qim->pixelColor( x, y ).lightness();
}

void rtimvClientBase::mtxUL_recolor()
{
    sharedLockT lock( m_calMutex );
    mtxL_recolor( lock );
}

void rtimvClientBase::mtxL_recolor( const sharedLockT &lock )
{
    static_cast<void>( lock );

    SHARED_CONN_LOCK

    auto request = std::make_shared<remote_rtimv::RecolorRequest>();
    auto response = std::make_shared<remote_rtimv::RecolorResponse>();
    auto *context = new grpc::ClientContext;

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
        if( m_shuttingDown )
        {
            delete context;
            return;
        }
        m_emptyRpcContexts.push_back( context );
        ++m_emptyRpcPending;
    }

    context->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );
    stub_->async()->Recolor( context,
                             request.get(),
                             response.get(),
                             [this, context, request, response]( grpc::Status status )
                             { this->EmptyRpc_callback( context, status ); } );
}

uint32_t rtimvClientBase::saturated()
{
    return m_saturated;
}

float rtimvClientBase::minImageData()
{
    return m_minImageData;
}

float rtimvClientBase::maxImageData()
{
    return m_maxImageData;
}

float rtimvClientBase::zoomLevel()
{
    return m_zoomLevel;
}

float rtimvClientBase::zoomLevelMin()
{
    return m_zoomLevelMin;
}

float rtimvClientBase::zoomLevelMax()
{
    return m_zoomLevelMax;
}

void rtimvClientBase::zoomLevel( float zl )
{
    if( zl < m_zoomLevelMin )
    {
        zl = m_zoomLevelMin;
    }

    if( zl > m_zoomLevelMax )
    {
        zl = m_zoomLevelMax;
    }

    m_zoomLevel = zl;

    RTIMV_DEBUG_BREADCRUMB

    post_zoomLevel();

    RTIMV_DEBUG_BREADCRUMB
}

bool rtimvClientBase::realTimeStopped()
{
    // get from server
    return false;
}

void rtimvClientBase::realTimeStopped( bool rts )
{
    // send to server
}

void rtimvClientBase::updateFPS()
{
    return;
}

void rtimvClientBase::updateAge()
{
    return;
}

void rtimvClientBase::updateNC()
{
    return;
}
