#include "rtimvClientBase.hpp"

// #define RTIMV_DEBUG_BREADCRUMB std::cerr << __FILE__ << " " << __LINE__ << "\n";
#define RTIMV_DEBUG_BREADCRUMB

rtimvClientBase::rtimvClientBase()
{
    m_foundation = new rtimvBaseObject( this, nullptr );
}

rtimvClientBase::~rtimvClientBase()
{
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

int rtimvClientBase::Configure()
{
    if( !m_configReq )
    {
        return -1;
    }

    // Container for the data we expect from the server.
    remote_rtimv::ConfigResult result;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->Configure( &context, *m_configReq, &result );

    // Act upon its status.
    if( status.ok() )
    {
        return result.result();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return -1;
    }
}

using namespace std::chrono_literals;

void rtimvClientBase::ImagePlease()
{
    std::lock_guard<std::mutex> lock( m_imageRequestMutex );

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

    // The actual RPC.
    stub_->async()->ImagePlease( m_ImagePleaseContext,
                                 &m_grpcImageRequest,
                                 &m_grpcImage,
                                 [this]( grpc::Status status ) { this->ImagePlease_callback( status ); } );

    m_imageRequestPending = true;
}

void rtimvClientBase::ImageReceived()
{
    uint32_t nx, ny, nz;
    float fpsEst;
    double imageTime;

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

        m_qim->loadFromData(
            reinterpret_cast<const uchar *>( m_grpcImage.image().data() ), m_grpcImage.image().size(), "jpeg" );

        imageTime = m_grpcImage.atime();
        fpsEst = m_grpcImage.fps();
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

    mtxL_postRecolor( ulock );

    ulock.unlock();
    sharedLockT slock( dummy );
    mtxL_postChangeImdata( slock );

    if( resized )
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

    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );

        if( !m_imageRequestPending )
        {
            std::cerr << "bug: in ImagePlease_callback but imageRequestPending is false " << __FILE__ << ' ' << __LINE__
                      << '\n';
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
                action = 1;
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_NOT_CONFIGURED )
            {
                // reconfigure
            }
            else if( m_grpcImage.status() == remote_rtimv::IMAGE_STATUS_ERROR )
            {
                // reconnect
            }
        }
        else
        {
            // reconnect
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            // return -1;
        }

        delete m_ImagePleaseContext;
        m_ImagePleaseContext = nullptr;

        m_imageRequestPending = false;
    }

    if( action == 0 )
    {
        m_foundation->emit_ImageWaiting();
    }
    else if( action == 1 )
    {
        m_foundation->emit_ImageNeeded();
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

std::string rtimvClientBase::imageName( size_t n )
{
    // get from server?
    return "";
}

uint32_t rtimvClientBase::imageNo( size_t n )
{
    // get from server?
    return 0;
}

std::vector<std::string> rtimvClientBase::info( size_t n )
{
    // get from server (maybe once and cache?)
    return std::vector<std::string>( { "" } );
}

void rtimvClientBase::mtxL_setImsize( uint32_t x, uint32_t y, uint32_t z, const uniqueLockT &lock )
{

    m_nz = z;
    // m_foundation->emit_nzUpdated( m_nz );

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
    // send to server
}

void rtimvClientBase::cubeFPS( float fps )
{
    // send to server
}

void rtimvClientBase::cubeFPSMult( float mult )
{
    // send to server
}

void rtimvClientBase::cubeDir( int dir )
{
    m_cubeDir = dir;
    // m_foundation->emit_cubeDirUpdated( m_cubeDir );
}

void rtimvClientBase::cubeFrame( uint32_t fno )
{
    // send to server
}

void rtimvClientBase::cubeFrameDelta( int32_t dfno )
{
    // send to server
}

void rtimvClientBase::updateImages()
{
    // If we aren't waiting on an image yet, sent ImagePlease
    /*if( !m_imageWaiting )
    {
        ImagePlease();
    }*/
}

void rtimvClientBase::updateCube()
{
    // send to server
}

void rtimvClientBase::updateCubeFrame()
{
    // m_foundation->emit_cubeFrameUpdated( imageNo() );
}

void rtimvClientBase::imageTimeout( int to )
{
    // send to server
}

int rtimvClientBase::imageTimeout()
{
    return m_imageTimeout;
}

void rtimvClientBase::subtractDark( bool sd )
{
    // send to server
}

bool rtimvClientBase::subtractDark()
{
    return m_subtractDark;
}

void rtimvClientBase::applyMask( bool amsk )
{
    // send to server
}

bool rtimvClientBase::applyMask()
{
    return m_applyMask;
}

void rtimvClientBase::applySatMask( bool asmsk )
{
    // send to server
}

bool rtimvClientBase::applySatMask()
{
    return m_applySatMask;
}

float rtimvClientBase::calPixel( uint32_t x, uint32_t y )
{
    // Data we are sending to the server.
    remote_rtimv::Coord request;
    request.set_x( x );
    request.set_y( y );

    // Container for the data we expect from the server.
    remote_rtimv::Pixel pixel;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->GetPixel( &context, request, &pixel );

    // Act upon its status.
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
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return 0;
    }
}

rtimv::colorbar rtimvClientBase::colorbar()
{
    return m_colorbar;
}

void rtimvClientBase::colormode( rtimv::colormode mode )
{
    m_colormode = mode;
}

rtimv::colormode rtimvClientBase::colormode()
{
    return m_colormode;
}

void rtimvClientBase::stretch( rtimv::stretch ct )
{
    m_stretch = ct;
}

rtimv::stretch rtimvClientBase::stretch()
{
    return m_stretch;
}

void rtimvClientBase::minScaleData( float md )
{
    // send to server
}

float rtimvClientBase::minScaleData()
{
    // get from server
    return 0;
}

void rtimvClientBase::maxScaleData( float md )
{
    // send to server
}

float rtimvClientBase::maxScaleData()
{
    // get from server
    return 0;
}

void rtimvClientBase::bias( float b )
{
    // send to server
}

float rtimvClientBase::bias()
{
    // get from server
    return 0;
}

void rtimvClientBase::bias_rel( float br )
{
    // send to server
}

float rtimvClientBase::bias_rel()
{
    // get from server
    return 0;
}

void rtimvClientBase::contrast( float c )
{
    // send to server
}

float rtimvClientBase::contrast()
{
    // get from server
    return 0;
}

float rtimvClientBase::contrast_rel()
{
    // get from server
    return 0;
}

void rtimvClientBase::contrast_rel( float cr )
{
    // send to server
}

uint8_t rtimvClientBase::lightness( int x, int y )
{
    if( !m_qim )
    {
        return 0;
    }

    return m_qim->pixelColor( x, y ).lightness();
}

void rtimvClientBase::mtxL_recolor( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    // send to server

    /* Unlike rtimvBase, we don't call mtxL_postRecolor( lock )
       because that will get called when the new image is retrieved
    */
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

void rtimvClientBase::colorBox_i0( int64_t i0 )
{
    // send to server
}

int64_t rtimvClientBase::colorBox_i0()
{
    // get from server
    return 0;
}

void rtimvClientBase::colorBox_i1( int64_t i1 )
{
    // send to server
}

int64_t rtimvClientBase::colorBox_i1()
{
    // get from server
    return 0;
}

void rtimvClientBase::colorBox_j0( int64_t j0 )
{
    // send to server
}

int64_t rtimvClientBase::colorBox_j0()
{
    // get from server
    return 0;
}

void rtimvClientBase::colorBox_j1( int64_t j1 )
{
    // send to server
}

int64_t rtimvClientBase::colorBox_j1()
{
    // get from server
    return 0;
}

void rtimvClientBase::mtxL_setColorBoxActive( bool usba, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    // send to server
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
