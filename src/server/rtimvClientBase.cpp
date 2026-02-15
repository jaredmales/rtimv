/** \file rtimvClientBase.cpp
 * \brief Definitions for the rtimvClientBase class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvClientBase.hpp"
#include "rtimvColorGRPC.hpp"

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
    if( connections == m_connections )                                                                                 \
    {                                                                                                                  \
        m_connected = false;                                                                                           \
                                                                                                                       \
        std::cerr << "Message from rtimvServer: " << status.error_message() << std::endl;                              \
    }

rtimvClientBase::rtimvClientBase()
{
    m_foundation = new rtimvBaseObject( this, nullptr );
    m_foundation->m_connectionTimer.start( 1000 );
}

rtimvClientBase::~rtimvClientBase()
{
    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        m_shuttingDown = true;

        if( m_ImagePleaseContext )
        {
            m_ImagePleaseContext->TryCancel();
        }
    }

    {
        std::unique_lock<std::mutex> lock( m_imageRequestMutex );
        while( m_imageRequestPending )
        {
            if( m_imageRequestCv.wait_for( lock, std::chrono::seconds( 1 ) ) == std::cv_status::timeout )
            {
                std::cerr << "rtimvClient: waiting for ImagePlease callback during shutdown.\n";
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
    config.add( "localConfig",
                "l",
                "localConfig",
                mx::app::argType::Required,
                "",
                "localConfig",
                false,
                "string",
                "A local configuration file." );

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
}

void rtimvClientBase::loadStandardConfig()
{
    config( m_configPathCL, "localConfig" );
    m_configPathCL = m_configPathCLBase + m_configPathCL;
}

void rtimvClientBase::loadConfig()
{
    if( m_configReq )
    {
        delete m_configReq;
    }

    m_configReq = new remote_rtimv::Config;

    if( config.isSet( "config" ) )
    {
        std::string configFile;
        config( configFile, "config" );
        m_configReq->set_file( configFile );
    }

    config( m_server, "server" );
    config( m_port, "port" );

    if( config.isSet( "image.key" ) )
    {
        std::string imKey;
        config( imKey, "image.key" );
        m_configReq->set_image_key( imKey );
    }

    if( config.isSet( "dark.key" ) )
    {
        std::string darkKey;
        config( darkKey, "dark.key" );
        m_configReq->set_dark_key( darkKey );
    }

    if( config.isSet( "flat.key" ) )
    {
        std::string flatKey;
        config( flatKey, "flat.key" );
        m_configReq->set_mask_key( flatKey );
    }

    if( config.isSet( "mask.key" ) )
    {
        std::string maskKey;
        config( maskKey, "mask.key" );
        m_configReq->set_mask_key( maskKey );
    }

    if( config.isSet( "satMask.key" ) )
    {
        std::string satMaskKey;
        config( satMaskKey, "satMask.key" );
        m_configReq->set_sat_mask_key( satMaskKey );
    }

    // The command line always overrides the config
    if( config.nonOptions.size() > 0 )
    {
        m_configReq->set_image_key( config.nonOptions[0] );
    }

    if( config.nonOptions.size() > 1 )
    {
        m_configReq->set_dark_key( config.nonOptions[1] );
        // darkKey = config.nonOptions[1]
    }

    if( config.nonOptions.size() > 2 )
    {
        m_configReq->set_mask_key( config.nonOptions[2] );
    }

    if( config.nonOptions.size() > 3 )
    {
        m_configReq->set_flat_key( config.nonOptions[3] );
    }

    if( config.nonOptions.size() > 4 )
    {
        m_configReq->set_sat_mask_key( config.nonOptions[4] );
    }

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
    Configure();

    m_foundation->emit_ImageNeeded();
}

bool rtimvClientBase::connected()
{
    return m_connected;
}

void rtimvClientBase::reconnect()
{
    sharedLockT lock( m_connectedMutex );

    if( m_connected )
    {
        return;
    }

    lock.unlock();

    Configure();

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
        m_connected = true;
        std::cerr << "rtimvClient connected to: " << m_server << ':' << m_port << '\n';
        m_connectionFailReported = false;
    }
    else
    {
        m_connected = false;
        if( !m_connectionFailReported )
        {
            std::cerr << "rtimvClient: " << status.error_message() << std::endl;
            m_connectionFailReported = true;
        }
    }
}

void rtimvClientBase::ImagePlease()
{
    updateAge();

    SHARED_CONN_LOCK

    std::lock_guard<std::mutex> lock( m_imageRequestMutex );

    if( m_shuttingDown )
    {
        return;
    }

    if( m_imageRequestPending )
    {

        std::cerr << "bug: in ImagePlease but imageRequestPending is true " << __FILE__ << ' ' << __LINE__ << '\n';
        return;
    }

    if( m_ImagePleaseContext )
    {
        std::cerr << "bug: in ImagePlease but ImagePleaseContext is allocated " << __FILE__ << ' ' << __LINE__ << '\n';
        return;
    }

    m_ImagePleaseContext = new grpc::ClientContext;
    m_ImagePleaseContext->set_deadline( std::chrono::system_clock::now() + std::chrono::milliseconds( 2000 ) );

    // The actual RPC.
    stub_->async()->ImagePlease( m_ImagePleaseContext,
                                 &m_grpcImageRequest,
                                 &m_grpcImage,
                                 [this]( grpc::Status status ) { this->ImagePlease_callback( status ); } );

    m_imageRequestPending = true;
}

void rtimvClientBase::ImageReceived()
{
    {
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
    float minsc, maxsc, minim, maxim;
    int imageTimeout;

    remote_rtimv::Colorbar colorbar;
    remote_rtimv::Colormode colormode;
    remote_rtimv::Colorstretch stretch;

    bool autoScale;
    bool subtractDark;
    bool applyMask;
    bool applySatMask;
    int cubeDir;
    bool hasImagePayload{ false };

    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( m_imageRequestPending )
        {
            std::cerr << "bug: in ImageRecieved but imageRequestPending is true " << __FILE__ << ' ' << __LINE__
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
            hasImagePayload = m_qim->loadFromData(
                reinterpret_cast<const uchar *>( m_grpcImage.image().data() ), m_grpcImage.image().size(), "jpeg" );
        }

        imageTime = m_grpcImage.atime();
        fpsEst = m_grpcImage.fps();

        saturated = m_grpcImage.saturated();

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

        imageTimeout = m_grpcImage.image_timeout();
        cubeDir = m_grpcImage.cube_dir();
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
    m_imageTime = imageTime;
    m_imageNo = imageNo;
    m_saturated = saturated;
    m_minImageData = minim;
    m_maxImageData = maxim;
    m_minScaleData = minsc;
    m_maxScaleData = maxsc;

    m_colorbar = rtimv::grpc2colorbar( colorbar );
    m_colormode = rtimv::grpc2colormode( colormode );
    m_stretch = rtimv::grpc2stretch( stretch );

    m_autoScale = autoScale;

    m_subtractDark = subtractDark;
    m_applyMask = applyMask;
    m_applySatMask = applySatMask;
    m_imageTimeout = imageTimeout;
    if( m_cubeDir != cubeDir )
    {
        m_cubeDir = cubeDir;
        m_foundation->emit_cubeDirUpdated( m_cubeDir );
    }

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

    // We always go on and get the next one
    m_foundation->emit_ImageNeeded();
}

void rtimvClientBase::ImagePlease_callback( grpc::Status status )
{
    int action = -1;
    bool shuttingDown = false;

    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( !m_imageRequestPending )
        {
            std::cerr << "bug: in ImagePlease_callback but imageRequestPending is false \n";
            return;
        }

        // Act upon its status.
        if( status.ok() )
        {
            if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_VALID )
            {
                action = 0;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_NO_IMAGE )
            {
                action = 1;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_TIMEOUT )
            {
                action = 0;
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
            SHARED_CONN_LOCK
            REPORT_SERVER_DISCONNECTED
            // don't return here so deallocation happens
        }

        delete m_ImagePleaseContext;
        m_ImagePleaseContext = nullptr;

        m_imageRequestPending = false;
        shuttingDown = m_shuttingDown;
    }

    m_imageRequestCv.notify_all();

    if( shuttingDown )
    {
        return;
    }

    if( action == 0 )
    {
        m_foundation->emit_ImageWaiting();
    }
    else if( action == 1 )
    {
        m_foundation->emit_ImageNeeded();
        // updateAge();
    }

    // if action stays -1 we just return
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

std::string rtimvClientBase::imageName( size_t n )
{
    // get from server?
    return "";
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
    // get from server (maybe once and cache?)
    return std::vector<std::string>( { "" } );
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

    remote_rtimv::CubeDirRequest request;
    remote_rtimv::CubeDirResponse response;
    grpc::ClientContext context;

    request.set_dir( dir );

    grpc::Status status = stub_->CubeDir( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

void rtimvClientBase::cubeFrame( uint32_t fno )
{
    SHARED_CONN_LOCK

    remote_rtimv::CubeFrameRequest request;
    remote_rtimv::CubeFrameResponse response;
    grpc::ClientContext context;

    request.set_frame( fno );

    grpc::Status status = stub_->CubeFrame( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

void rtimvClientBase::cubeFrameDelta( int32_t dfno )
{
    SHARED_CONN_LOCK

    remote_rtimv::CubeFrameDeltaRequest request;
    remote_rtimv::CubeFrameDeltaResponse response;
    grpc::ClientContext context;

    request.set_delta( dfno );

    grpc::Status status = stub_->CubeFrameDelta( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

void rtimvClientBase::updateCube()
{
    SHARED_CONN_LOCK

    remote_rtimv::UpdateCubeRequest request;
    remote_rtimv::UpdateCubeResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->UpdateCube( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
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

void rtimvClientBase::imageTimeout( int to )
{
    SHARED_CONN_LOCK

    remote_rtimv::ImageTimeoutRequest request;
    remote_rtimv::ImageTimeoutResponse response;
    grpc::ClientContext context;

    request.set_timeout( to );

    grpc::Status status = stub_->SetImageTimeout( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

int rtimvClientBase::imageTimeout()
{
    return m_imageTimeout;
}

void rtimvClientBase::subtractDark( bool sd )
{
    SHARED_CONN_LOCK

    remote_rtimv::SubDarkRequest request;
    remote_rtimv::SubDarkResponse response;
    grpc::ClientContext context;

    request.set_subtract_dark( sd );

    grpc::Status status = stub_->SetSubDark( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

bool rtimvClientBase::subtractDark()
{
    return m_subtractDark;
}

void rtimvClientBase::applyMask( bool amsk )
{
    SHARED_CONN_LOCK

    remote_rtimv::ApplyMaskRequest request;
    remote_rtimv::ApplyMaskResponse response;
    grpc::ClientContext context;

    request.set_apply_mask( amsk );

    grpc::Status status = stub_->SetApplyMask( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

bool rtimvClientBase::applyMask()
{
    return m_applyMask;
}

void rtimvClientBase::applySatMask( bool asmsk )
{
    SHARED_CONN_LOCK

    remote_rtimv::ApplySatMaskRequest request;
    remote_rtimv::ApplySatMaskResponse response;
    grpc::ClientContext context;

    request.set_apply_sat_mask( asmsk );

    grpc::Status status = stub_->SetApplySatMask( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
    }
}

bool rtimvClientBase::applySatMask()
{
    return m_applySatMask;
}

float rtimvClientBase::calPixel( uint32_t x, uint32_t y )
{
    SHARED_CONN_LOCK_RET( 0 )

    remote_rtimv::Coord request;
    remote_rtimv::Pixel pixel;
    grpc::ClientContext context;

    request.set_x( x );
    request.set_y( y );

    grpc::Status status = stub_->GetPixel( &context, request, &pixel );

    if( status.ok() )
    {
        if( !pixel.valid() )
        {
            return 0;
        }

        return pixel.value();
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return 0;
    }
}

void rtimvClientBase::mtxL_load_colorbarImpl( rtimv::colorbar cb, bool update )
{
    SHARED_CONN_LOCK

    remote_rtimv::ColorbarRequest request;
    remote_rtimv::ColorbarResponse response;
    grpc::ClientContext context;

    request.set_colorbar( rtimv::colorbar2grpc( cb ) );
    request.set_update( update );

    grpc::Status status = stub_->SetColorbar( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
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

    SHARED_CONN_LOCK

    if( m == rtimv::colormode::minmaxbox )
    {
        remote_rtimv::Box request;
        remote_rtimv::MinvalMaxval vals;
        grpc::ClientContext context;

        remote_rtimv::Coord *ul = new remote_rtimv::Coord;
        ul->set_x( m_colorBox_i0 );
        ul->set_y( m_colorBox_j0 );
        request.set_allocated_upper_left( ul );

        remote_rtimv::Coord *lr = new remote_rtimv::Coord;
        lr->set_x( m_colorBox_i1 );
        lr->set_y( m_colorBox_j1 );
        request.set_allocated_lower_right( lr );

        grpc::Status status = stub_->ColorBox( &context, request, &vals );

        if( !status.ok() )
        {
            REPORT_SERVER_DISCONNECTED
            return;
        }

        if( !vals.valid() )
        {
            return;
        }

        m_colorBox_min = vals.min();
        m_colorBox_max = vals.max();
    }
    else
    {
        remote_rtimv::ColormodeRequest request;
        remote_rtimv::ColormodeResponse response;
        grpc::ClientContext context;

        request.set_colormode( rtimv::colormode2grpc( m ) );

        grpc::Status status = stub_->SetColormode( &context, request, &response );

        if( !status.ok() )
        {
            REPORT_SERVER_DISCONNECTED
            return;
        }
    }

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

void rtimvClientBase::stretch( rtimv::stretch cs )
{
    SHARED_CONN_LOCK

    remote_rtimv::ColorstretchRequest request;
    remote_rtimv::ColorstretchResponse response;
    grpc::ClientContext context;

    request.set_colorstretch( rtimv::stretch2grpc( cs ) );

    grpc::Status status = stub_->SetColorstretch( &context, request, &response );

    if( !status.ok() )
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
}

rtimv::stretch rtimvClientBase::stretch()
{
    return m_stretch;
}

void rtimvClientBase::minScaleData( float md )
{
    SHARED_CONN_LOCK

    remote_rtimv::ScaleRequest request;
    remote_rtimv::ScaleResponse response;
    grpc::ClientContext context;

    request.set_value( md );

    grpc::Status status = stub_->SetMinScale( &context, request, &response );

    if( status.ok() )
    {
        return;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
}

float rtimvClientBase::minScaleData()
{
    return m_minScaleData;
}

void rtimvClientBase::maxScaleData( float md )
{
    SHARED_CONN_LOCK

    remote_rtimv::ScaleRequest request;
    remote_rtimv::ScaleResponse response;
    grpc::ClientContext context;

    request.set_value( md );

    grpc::Status status = stub_->SetMaxScale( &context, request, &response );

    if( status.ok() )
    {
        return;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
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

    remote_rtimv::AutoscaleRequest request;
    remote_rtimv::AutoscaleResponse response;
    grpc::ClientContext context;

    request.set_autoscale( as );

    grpc::Status status = stub_->SetAutoscale( &context, request, &response );

    // Act upon its status.
    if( status.ok() )
    {
        return;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
}

bool rtimvClientBase::autoScale()
{
    return m_autoScale;
}

void rtimvClientBase::mtxUL_reStretch()
{
    SHARED_CONN_LOCK

    remote_rtimv::RestretchRequest request;
    remote_rtimv::RestretchResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->Restretch( &context, request, &response );

    if( status.ok() )
    {
        return;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
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

    remote_rtimv::RecolorRequest request;
    remote_rtimv::RecolorResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->Recolor( &context, request, &response );

    // Act upon its status.
    if( status.ok() )
    {
        return;
    }
    else
    {
        REPORT_SERVER_DISCONNECTED
        return;
    }
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
