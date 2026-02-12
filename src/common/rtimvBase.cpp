#include "rtimvBase.hpp"
#include "rtimvBaseObject.hpp"

#include <cmath>

#ifdef MXLIB_MILK
    #include "images/shmimImage.hpp"
#endif

#include "images/fitsImage.hpp"
#include "images/fitsDirectory.hpp"
#include "images/mzmqImage.hpp"

// #define RTIMV_DEBUG_BREADCRUMB std::cerr << __FILE__ << " " << __LINE__ << "\n";
#define RTIMV_DEBUG_BREADCRUMB

rtimvBase::rtimvBase()
{
    m_foundation = new rtimvBaseObject( this, nullptr );
}

rtimvBase::~rtimvBase()
{
    for( size_t n = 0; n < m_images.size(); ++n )
    {
        if( m_images[n] )
        {
            m_images[n]->deleteLater();
        }
    }

    if( m_foundation )
    {
        m_foundation->deleteLater();
    }
}

void rtimvBase::setupConfig()
{
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

void rtimvBase::loadConfig()
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

    // Check for use of deprecated shmim_name keyword by itself, but use key if available
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

    processKeys( keys );

    if( m_images[0] == nullptr )
    {
        if( doHelp )
        {
            help();

            throw std::runtime_error( "help" );
        }
        else
        {
            throw std::runtime_error(
                "rtimv: No valid image specified so cowardly refusing to start.  Use -h for help." );
        }
    }

    // Now load remaining options, respecting coded defaults.

    // get timeouts.
    float fps = -999;
    config( fps, "update.fps" );

    if( fps > 0 ) // fps sets m_imageTimeout
    {
        m_imageTimeout = std::round( 1000. / fps );
    }

    // but update.timeout can override it
    config( m_imageTimeout, "update.timeout" );
    config( m_cubeFPS, "update.cubeFPS" );

    // Now set the actual timeouts
    cubeFPS( m_cubeFPS );

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

void rtimvBase::processKeys( const std::vector<std::string> &shkeys )
{
    m_images.resize( 4, nullptr );

    for( size_t i = 0; i < m_images.size(); ++i )
    {
        if( shkeys.size() > i )
        {
            if( shkeys[i] != "" )
            {
                // safely accept several different common fits extensions
                bool isFits = false;
                if( shkeys[i].size() > 4 )
                {
                    if( shkeys[i].rfind( ".fit" ) == shkeys[i].size() - 4 ||
                        shkeys[i].rfind( ".FIT" ) == shkeys[i].size() - 4 )
                    {
                        isFits = true;
                    }
                }
                if( shkeys[i].size() > 5 && !isFits )
                {
                    if( shkeys[i].rfind( ".fits" ) == shkeys[i].size() - 5 ||
                        shkeys[i].rfind( ".FITS" ) == shkeys[i].size() - 5 )
                    {
                        isFits = true;
                    }
                }

                bool isDirectory = false;

                if( !isFits )
                {
                    if( shkeys[i][shkeys[i].size() - 1] == '/' )
                    {
                        isDirectory = true;
                    }
                }

                if( isFits )
                {
                    fitsImage *fi = new fitsImage( &m_rawMutex );
                    m_images[i] = (rtimvImage *)fi;
                }
                else if( isDirectory )
                {
                    fitsDirectory *fd = new fitsDirectory( &m_rawMutex );
                    m_images[i] = (rtimvImage *)fd;
                }
                else if( shkeys[i].find( '@' ) != std::string::npos || shkeys[i].find( ':' ) != std::string::npos ||
                         m_mzmqAlways == true )
                {
                    mzmqImage *mi = new mzmqImage( &m_rawMutex );

                    // change defaults
                    if( m_mzmqServer != "" )
                    {
                        mi->imageServer( m_mzmqServer );
                    }

                    if( m_mzmqPort != 0 )
                    {
                        mi->imagePort( m_mzmqPort );
                    }

                    m_images[i] = (rtimvImage *)mi;
                }
                else
                {
                    // clang-format off
                    #ifdef MXLIB_MILK
                        // If we get here we try to interpret as an ImageStreamIO image
                        shmimImage *si = new shmimImage( &m_rawMutex );
                        m_images[i] = (rtimvImage *)si;
                    #else
                        qFatal( "Unrecognized image key format" );
                    #endif
                    // clang-format on
                }

                m_images[i]->imageKey( shkeys[i] ); // Set the key
            }
        }
    }

    // Turn on features if images exist:
    if( m_images[1] != nullptr )
    {
        m_subtractDark = true;
    }

    if( m_images[2] != nullptr )
    {
        m_applyMask = true;
    }

    if( m_images[3] != nullptr )
    {
        m_applySatMask = true;
    }
}

void rtimvBase::startup()
{
    m_foundation->m_imageTimer.start( m_imageTimeout );
}

void rtimvBase::onConnect()
{
    m_connected = true;
}

bool rtimvBase::connected()
{
    return m_connected;
}

bool rtimvBase::imageValid()
{
    return imageValid( 0 );
}

bool rtimvBase::imageValid( size_t n )
{
    if( n >= m_images.size() )
    {
        return false;
    }

    if( m_images[n] == nullptr )
    {
        return false;
    }

    return m_images[n]->valid();
}

double rtimvBase::imageTime()
{
    return imageTime( 0 );
}

double rtimvBase::imageTime( size_t n )
{
    if( !imageValid( n ) )
    {
        return 0;
    }

    return m_images[n]->imageTime();
}

double rtimvBase::fpsEst()
{
    return fpsEst( 0 );
}

double rtimvBase::fpsEst( size_t n )
{
    if( !imageValid( n ) )
    {
        return 0;
    }

    return m_images[n]->fpsEst();
}

std::string rtimvBase::imageName( size_t n )
{
    if( !imageValid( n ) )
    {
        return std::string();
    }

    return m_images[n]->imageName();
}

uint32_t rtimvBase::imageNo( size_t n )
{
    if( !imageValid( n ) )
    {
        return 0;
    }

    return m_images[n]->imageNo();
}

std::vector<std::string> rtimvBase::info( size_t n )
{
    if( !imageValid( n ) )
    {
        return std::vector<std::string>();
    }

    return m_images[n]->info();
}

void rtimvBase::mtxL_setImsize( uint32_t x, uint32_t y, uint32_t z, const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    // Always have at least one pixel
    if( x == 0 )
    {
        x = 1;
    }

    if( y == 0 )
    {
        y = 1;
    }

    if( z == 0 )
    {
        z = 1;
    }

    m_nz = z;
    m_foundation->emit_nzUpdated( m_nz );

    if( m_nx != x || m_ny != y || m_calData == 0 || m_qim == 0 )
    {
        m_nx = x;
        m_ny = y;

        if( m_calData != nullptr )
        {
            delete[] m_calData;
            m_calData = nullptr;
        }

        if( m_satData != nullptr )
        {
            delete[] m_satData;
            m_satData = nullptr;
        }

        m_calData = new float[m_nx * m_ny];
        m_satData = new uint8_t[m_nx * m_ny];

        if( m_qim != nullptr )
        {
            delete m_qim;
            m_qim = nullptr;
        }

        m_qim = new QImage( m_nx, m_ny, QImage::Format_Indexed8 );

        mtxL_load_colorbar( m_colorbar, false, lock ); // have to load into newly created image

        mtxL_postSetImsize( lock );
    }
}

uint32_t rtimvBase::nx()
{
    return m_nx;
}

uint32_t rtimvBase::ny()
{
    return m_ny;
}

uint32_t rtimvBase::nz()
{
    return m_nz;
}

void rtimvBase::setCurrImageTimeout()
{
    int cubeTimeout;
    int currImageTimeout;

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

        currImageTimeout = m_imageTimeout;
    }
    else // it's a cube, cube mode is on, and FPS > 0
    {
        // First get our wish
        cubeTimeout = std::round( 1000. / ( m_desiredCubeFPS * m_cubeFPSMult ) );

        if( cubeTimeout < 1 )
        {
            cubeTimeout = 1;
        }

        if( cubeTimeout <= m_imageTimeout )
        {
            // Report reality
            m_cubeFPS = ( 1000.0 / cubeTimeout ) / m_cubeFPSMult;
            currImageTimeout = cubeTimeout;
        }
        else
        {
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
            currImageTimeout = m_imageTimeout;
        }

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

    if( currImageTimeout == m_currImageTimeout ) // Don't interrupt if not needed
    {
        return;
    }

    m_currImageTimeout = currImageTimeout;

    m_foundation->m_imageTimer.stop();

    for( size_t i = 0; i < m_images.size(); ++i )
    {
        if( m_images[i] != nullptr )
        {
            m_images[i]->timeout( m_currImageTimeout ); // just for fps calculations
        }
    }

    m_foundation->m_imageTimer.start( m_currImageTimeout );
}

void rtimvBase::imageTimeout( int to )
{
    m_imageTimeout = to;

    setCurrImageTimeout();
}

int rtimvBase::imageTimeout()
{
    return m_imageTimeout;
}

void rtimvBase::cubeMode( bool cm )
{
    m_cubeMode = cm;

    setCurrImageTimeout();

    m_foundation->emit_cubeModeUpdated( m_cubeMode );
}

void rtimvBase::cubeFPS( float fps )
{
    if( fps < 0 )
    {
        fps = 0;
    }
    m_desiredCubeFPS = fps;
    setCurrImageTimeout();

    m_foundation->emit_cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
}

void rtimvBase::cubeFPSMult( float mult )
{
    m_cubeFPSMult = mult;
    setCurrImageTimeout();
    m_foundation->emit_cubeFPSMultUpdated( m_cubeFPSMult );
}

void rtimvBase::cubeDir( int dir )
{
    m_cubeDir = dir;
    m_foundation->emit_cubeDirUpdated( m_cubeDir );
}

void rtimvBase::cubeFrame( uint32_t fno )
{
    if( m_images[0] != nullptr )
    {
        m_images[0]->imageNo( fno );
    }
}

void rtimvBase::cubeFrameDelta( int32_t dfno )
{
    if( m_images[0] != nullptr )
    {
        m_images[0]->deltaImageNo( dfno );
    }
}

void rtimvBase::updateImages()
{
    int doupdate = RTIMVIMAGE_NOUPDATE;
    int supportUpdate = RTIMVIMAGE_NOUPDATE;

    if( m_images[0] != nullptr )
    {
        doupdate = m_images[0]->update();
    }

    for( size_t i = 1; i < m_images.size(); ++i )
    {
        if( m_images[i] != nullptr )
        {
            int sU = m_images[i]->update();
            if( sU > supportUpdate ) // Do an update if any support image needs an update
            {
                supportUpdate = sU;
            }
        }
    }

    ///\todo onConnect may need to wait a sec to let things settle.
    if( doupdate >= RTIMVIMAGE_IMUPDATE || supportUpdate >= RTIMVIMAGE_IMUPDATE )
    {
        mtxUL_changeImdata();

        if( doupdate >= RTIMVIMAGE_IMUPDATE && m_images[0]->nz() > 1 )
        {
            updateCubeFrame();
        }

        if( !m_connected && doupdate >= RTIMVIMAGE_IMUPDATE ) // this will only trigger onConnect on the main image
        {
            if( m_images[0]->nz() > 1 )
            {
                cubeMode( m_images[0]->defaultCubeMode() );
            }

            onConnect();
        }
    }

    if( !m_connected )
    {
        updateNC();
        return;
    }

    if( doupdate == RTIMVIMAGE_FPSUPDATE )
    {
        updateFPS();
    }

    if( doupdate == RTIMVIMAGE_AGEUPDATE )
    {
        updateAge();
    }
}

void rtimvBase::updateCube()
{
    if( m_images[0] != nullptr )
    {
        if( m_cubeDir >= 0 )
        {
            m_images[0]->incImageNo();
        }
        else
        {
            m_images[0]->decImageNo();
        }
    }
}

void rtimvBase::updateCubeFrame()
{
    m_foundation->emit_cubeFrameUpdated( m_images[0]->imageNo() );
}

void rtimvBase::subtractDark( bool sd )
{
    if( sd != m_subtractDark )
    {
        m_subtractDark = sd;
        mtxUL_changeImdata(); // have to trigger refresh of cal data
    }
}
bool rtimvBase::subtractDark()
{
    return m_subtractDark;
}
void rtimvBase::applyMask( bool amsk )
{
    if( amsk != m_applyMask )
    {
        m_applyMask = amsk;
        mtxUL_changeImdata(); // have to trigger refresh of cal data
    }
}
bool rtimvBase::applyMask()
{
    return m_applyMask;
}
void rtimvBase::applySatMask( bool asmsk )
{
    if( asmsk != m_applySatMask )
    {
        m_applySatMask = asmsk;
        mtxUL_changeImdata(); // have to trigger refresh of cal data
    }
}
bool rtimvBase::applySatMask()
{
    return m_applySatMask;
}

rtimvBase::pixelF rtimvBase::rawPixel()
{
    pixelF _pixel = nullptr;

    if( m_images[0] == nullptr )
    {
        return _pixel; // no valid base image
    }

    if( m_images[0]->valid() )
    {
        _pixel = &pixel_noCal; // default if there is a valid base image.
    }
    else
    {
        return _pixel; // no valid base image
    }

    if( m_subtractDark == true && m_applyMask == false )
    {
        if( m_images[1] == nullptr )
        {
            return _pixel;
        }

        if( m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny() )
        {
            return _pixel;
        }

        if( m_images[0]->valid() && m_images[1]->valid() )
        {
            _pixel = &pixel_subDark;
        }
    }

    if( m_subtractDark == false && m_applyMask == true )
    {
        if( m_images[2] == nullptr )
            return _pixel;

        if( m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny() )
            return _pixel;

        if( m_images[0]->valid() && m_images[2]->valid() )
            _pixel = &pixel_applyMask;
    }

    if( m_subtractDark == true && m_applyMask == true )
    {

        if( m_images[1] == nullptr && m_images[2] == nullptr )
            return _pixel;
        else if( m_images[2] == nullptr )
        {
            if( m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny() )
                return _pixel;
            if( m_images[1]->valid() )
                _pixel = &pixel_subDark;
        }
        else if( m_images[1] == nullptr )
        {
            if( m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny() )
                return _pixel;
            if( m_images[2]->valid() )
                _pixel = &pixel_applyMask;
        }
        else
        {
            if( m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny() )
                return _pixel;
            if( m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny() )
                return _pixel;
            if( m_images[1]->valid() && m_images[2]->valid() )
                _pixel = &pixel_subDarkApplyMask;
        }
    }

    return _pixel;
}

float rtimvBase::pixel_noCal( rtimvBase *imv, size_t idx )
{
    return imv->m_images[0]->pixel( idx );
}

float rtimvBase::pixel_subDark( rtimvBase *imv, size_t idx )
{
    return imv->m_images[0]->pixel( idx ) - imv->m_images[1]->pixel( idx );
}

float rtimvBase::pixel_applyMask( rtimvBase *imv, size_t idx )
{
    return imv->m_images[0]->pixel( idx ) * imv->m_images[2]->pixel( idx );
}

float rtimvBase::pixel_subDarkApplyMask( rtimvBase *imv, size_t idx )
{
    return ( imv->m_images[0]->pixel( idx ) - imv->m_images[1]->pixel( idx ) ) * imv->m_images[2]->pixel( idx );
}

float rtimvBase::calPixel( uint32_t x, uint32_t y )
{
    return m_calData[y * m_nx + x];
}

uint8_t rtimvBase::satPixel( uint32_t x, uint32_t y )
{
    return m_satData[y * m_nx + x];
}

void rtimvBase::mtxL_load_colorbarImpl( rtimv::colorbar cb )
{
    if( !m_qim )
    {
        return;
    }

    m_colorbar = cb;
    switch( cb )
    {
    case rtimv::colorbar::jet:
        m_minColor = 0;
        m_maxColor = load_colorbar_jet( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "white" );
        break;
    case rtimv::colorbar::hot:
        m_minColor = 0;
        m_maxColor = load_colorbar_hot( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "cyan" );
        break;
    case rtimv::colorbar::bone:
        m_minColor = 0;
        m_maxColor = load_colorbar_bone( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "red" );
        break;
    case rtimv::colorbar::red:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for( int i = m_minColor; i <= m_maxColor; i++ )
        {
            int c = ( ( (float)i ) / 253. * 255. ) + 0.5;
            m_qim->setColor( i, qRgb( c, 0, 0 ) );
        }
        m_qim->setColor( 254, qRgb( 0, 0, 0 ) );
        m_qim->setColor( 255, qRgb( 0, 255, 0 ) );
        warning_color = QColor( "red" );
        break;
    case rtimv::colorbar::green:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for( int i = m_minColor; i <= m_maxColor; i++ )
        {
            int c = ( ( (float)i ) / 253. * 255. ) + 0.5;
            m_qim->setColor( i, qRgb( 0, c, 0 ) );
        }
        m_qim->setColor( 254, qRgb( 0, 0, 0 ) );
        m_qim->setColor( 255, qRgb( 255, 0, 0 ) );
        warning_color = QColor( "red" );
        break;
    case rtimv::colorbar::blue:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for( int i = m_minColor; i <= m_maxColor; i++ )
        {
            int c = ( ( (float)i ) / 253. * 255. ) + 0.5;
            m_qim->setColor( i, qRgb( 0, 0, c ) );
        }
        m_qim->setColor( 254, qRgb( 0, 0, 0 ) );
        m_qim->setColor( 255, qRgb( 255, 0, 0 ) );
        warning_color = QColor( "red" );
        break;
    default:
        m_colorbar = rtimv::colorbar::grey;
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for( int i = m_minColor; i <= m_maxColor; i++ )
        {
            int c = ( ( (float)i ) / 253. * 255. ) + 0.5;
            m_qim->setColor( i, qRgb( c, c, c ) );
        }
        m_qim->setColor( 254, qRgb( 0, 0, 0 ) );
        m_qim->setColor( 255, qRgb( 255, 0, 0 ) );

        warning_color = QColor( "red" );
        break;
    }

    m_lightness.resize( 256 );

    for( int n = 0; n < 256; ++n )
    {
        m_lightness[n] = QColor( m_qim->color( n ) ).lightness();
    }
}

void rtimvBase::mtxL_load_colorbar( rtimv::colorbar cb, bool update, const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_load_colorbarImpl( cb );

    if( update )
    {
        mtxL_recolorImpl();
        mtxL_postRecolor( lock );
    }
}

void rtimvBase::mtxL_load_colorbar( rtimv::colorbar cb, bool update, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_load_colorbarImpl( cb );

    if( update )
    {
        mtxL_recolorImpl();
        mtxL_postRecolor( lock );
    }
}

void rtimvBase::mtxUL_load_colorbar( rtimv::colorbar cb, bool update )
{
    sharedLockT lock( m_calMutex );

    mtxL_load_colorbar( cb, update, lock );
}

rtimv::colorbar rtimvBase::colorbar()
{
    return m_colorbar;
}

void rtimvBase::mtxUL_colormode( rtimv::colormode m )
{
    sharedLockT lock( m_calMutex );

    mtxL_colormode( m, lock );
}

void rtimvBase::mtxL_colormode( rtimv::colormode m, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    if( m == rtimv::colormode::minmaxbox )
    {
        float imval;

        normalizeColorBox();

        m_colorBox_min = std::numeric_limits<float>::max();
        m_colorBox_max = -std::numeric_limits<float>::max();

        for( int i = m_colorBox_i0; i <= m_colorBox_i1; i++ )
        {
            for( int j = m_colorBox_j0; j <= m_colorBox_j1; j++ )
            {
                imval = calPixel( i, j );

                if( !std::isfinite( imval ) )
                {
                    continue;
                }

                if( imval < m_colorBox_min )
                {
                    m_colorBox_min = imval;
                }
                if( imval > m_colorBox_max )
                {
                    m_colorBox_max = imval;
                }
            }
        }

        if( m_colorBox_min == std::numeric_limits<float>::max() &&
            m_colorBox_max == -std::numeric_limits<float>::max() ) // If all nans
        {
            m_colorBox_min = 0;
            m_colorBox_max = 0;
        }

        minScaleData( m_colorBox_min );
        maxScaleData( m_colorBox_max );

        m_colormode = rtimv::colormode::minmaxbox;

        RTIMV_DEBUG_BREADCRUMB
    }
    else
    {
        m_colormode = m;
    }

    mtxL_recolorImpl();

    RTIMV_DEBUG_BREADCRUMB

    mtxL_postRecolor( lock );

    RTIMV_DEBUG_BREADCRUMB

    mtxL_postColormode( m, lock );

    RTIMV_DEBUG_BREADCRUMB
}

rtimv::colormode rtimvBase::colormode()
{
    return m_colormode;
}

void rtimvBase::colorBox_i0( int64_t i0 )
{
    m_colorBox_i0 = i0;
}

int64_t rtimvBase::colorBox_i0()
{
    return m_colorBox_i0;
}

void rtimvBase::colorBox_j0( int64_t j0 )
{
    m_colorBox_j0 = j0;
}

int64_t rtimvBase::colorBox_j0()
{
    return m_colorBox_j0;
}

void rtimvBase::colorBox_j1( int64_t j1 )
{
    m_colorBox_j1 = j1;
}

void rtimvBase::colorBox_i1( int64_t i1 )
{
    m_colorBox_i1 = i1;
}

int64_t rtimvBase::colorBox_i1()
{
    return m_colorBox_i1;
}

int64_t rtimvBase::colorBox_j1()
{
    return m_colorBox_j1;
}

float rtimvBase::colorBox_min()
{
    return m_colorBox_min;
}

float rtimvBase::colorBox_max()
{
    return m_colorBox_max;
}

void rtimvBase::stretch( rtimv::stretch ct )
{
    m_stretch = ct;
}

rtimv::stretch rtimvBase::stretch()
{
    return m_stretch;
}

void rtimvBase::minScaleData( float md )
{
    m_minScaleData = md;
}

float rtimvBase::minScaleData()
{
    return m_minScaleData;
}

void rtimvBase::maxScaleData( float md )
{
    m_maxScaleData = md;
}

float rtimvBase::maxScaleData()
{
    return m_maxScaleData;
}

void rtimvBase::bias( float b )
{
    float cont = contrast();

    minScaleData( b - 0.5 * cont );
    maxScaleData( b + 0.5 * cont );
}

float rtimvBase::bias()
{
    return 0.5 * ( m_maxScaleData + m_minScaleData );
}

void rtimvBase::bias_rel( float br )
{
    float cont = contrast();

    minScaleData( m_minImageData + br * ( m_maxImageData - m_minImageData ) - 0.5 * cont );
    maxScaleData( m_minImageData + br * ( m_maxImageData - m_minImageData ) + 0.5 * cont );
}

float rtimvBase::bias_rel()
{
    return 0.5 * ( m_maxScaleData + m_minScaleData ) / ( m_maxScaleData - m_minScaleData );
}

void rtimvBase::contrast( float c )
{
    float b = bias();
    minScaleData( b - 0.5 * c );
    maxScaleData( b + 0.5 * c );
}

float rtimvBase::contrast()
{
    return m_maxScaleData - m_minScaleData;
}

float rtimvBase::contrast_rel()
{
    return ( m_maxImageData - m_minImageData ) / ( m_maxScaleData - m_minScaleData );
}

void rtimvBase::contrast_rel( float cr )
{
    float b = bias();
    minScaleData( b - .5 * ( m_maxImageData - m_minImageData ) / cr );
    maxScaleData( b + .5 * ( m_maxImageData - m_minImageData ) / cr );
}

void rtimvBase::mtxUL_reStretch()
{
    if( colormode() == rtimv::colormode::user )
    {
        mtxUL_colormode( rtimv::colormode::minmaxglobal );
    }

    if( colormode() == rtimv::colormode::minmaxglobal )
    {
        minScaleData( minImageData() );
        maxScaleData( maxImageData() );
    }
    else if( colormode() == rtimv::colormode::minmaxbox )
    {
        minScaleData( m_colorBox_min );
        maxScaleData( m_colorBox_max );
    }

    sharedLockT lock( m_calMutex );
    mtxL_recolor( lock );
}

// Produce a value nominally between 0 and 1, though depending on the range it could be > 1.
#define NORMALIZE_PIXVAL( pixval, mindat, maxdat )                                                                     \
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );                                                     \
    if( pixval < 0 )                                                                                                   \
    {                                                                                                                  \
        return 0;                                                                                                      \
    }

// Clamp pixval to <= 1 and scale to the colorbar range
#define SCALE_PIXVAL_RETURN( pixval, mincol, maxcol )                                                                  \
    if( pixval > 1. )                                                                                                  \
    {                                                                                                                  \
        pixval = 1.;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    return mincol + pixval * ( maxcol - mincol ) + 0.5;

int calcPixIndex_linear( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    NORMALIZE_PIXVAL( pixval, mindat, maxdat );

    SCALE_PIXVAL_RETURN( pixval, mincol, maxcol );
}

int calcPixIndex_log( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    static float a = 1000;
    static float log10_a = log10( a );

    NORMALIZE_PIXVAL( pixval, mindat, maxdat );

    pixval = log10( pixval * a + 1 ) / log10_a;

    SCALE_PIXVAL_RETURN( pixval, mincol, maxcol );
}

int calcPixIndex_pow( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    static float a = 1000;

    NORMALIZE_PIXVAL( pixval, mindat, maxdat );

    pixval = ( pow( a, pixval ) ) / a;

    SCALE_PIXVAL_RETURN( pixval, mincol, maxcol );
}

int calcPixIndex_sqrt( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    NORMALIZE_PIXVAL( pixval, mindat, maxdat );

    pixval = sqrt( pixval );

    SCALE_PIXVAL_RETURN( pixval, mincol, maxcol );
}

int calcPixIndex_square( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    NORMALIZE_PIXVAL( pixval, mindat, maxdat );

    pixval = pixval * pixval;

    SCALE_PIXVAL_RETURN( pixval, mincol, maxcol );
}

const std::vector<uint8_t> &rtimvBase::lightness()
{
    return m_lightness;
}

uint8_t rtimvBase::lightness( size_t n )
{
    if( n > m_lightness.size() )
    {
        return 0;
    }

    return m_lightness[n];
}

uint8_t rtimvBase::lightness( int x, int y )
{
    size_t n = m_qim->pixelIndex( x, y );

    return lightness( n );
}

void rtimvBase::mtxUL_changeImdata()
{
    RTIMV_DEBUG_BREADCRUMB

    std::unique_lock changeLock{ m_changingImData, std::defer_lock };

    bool amChanging = !changeLock.try_lock();

    if( amChanging ) // this means we're already in this function
    {
        return;
    }

    bool resized = false;

    { // mutex scope

        // Get a unique lock on the cal data to allow us to delete it and overwrite it
        std::unique_lock<std::shared_mutex> lock( m_calMutex );

        // Also lock the raw data for the size checks to make sure it doesn't change out from under us
        std::unique_lock<std::mutex> rawlock( m_rawMutex );

        // Check again in case it changed while we were waiting on the lock
        if( !imageValid( 0 ) )
        {
            return;
        }

        // Here we realize we need to resize
        if( m_images[0]->nx() != m_nx || m_images[0]->ny() != m_ny || !m_qim )
        {
            RTIMV_DEBUG_BREADCRUMB

            mtxL_setImsize( m_images[0]->nx(), m_images[0]->ny(), m_images[0]->nz(), lock );

            RTIMV_DEBUG_BREADCRUMB

            resized = true;
        }

        RTIMV_DEBUG_BREADCRUMB

        // Get the pixel calculating function
        float ( *_pixel )( rtimvBase *, size_t ) = rawPixel();

        if( _pixel == nullptr )
        {
            return;
        }

        RTIMV_DEBUG_BREADCRUMB

        if( m_nx != m_images[0]->nx() || m_ny != m_images[0]->ny() )
        {
            return;
        }

        RTIMV_DEBUG_BREADCRUMB

        m_saturated = 0;
        for( uint64_t n = 0; n < m_nx * m_ny; ++n )
        {
            // Check for saturation
            if( pixel_noCal( this, n ) >= m_satLevel )
            {
                m_satData[n] = 1;
                ++m_saturated;
            }
            else
            {
                m_satData[n] = 0;
            }

            // Fill in calibrated value
            m_calData[n] = _pixel( this, n );
        }

        RTIMV_DEBUG_BREADCRUMB

        // Now check the sat image itself
        if( imageValid( 3 ) )
        {
            if( m_nx == m_images[3]->nx() && m_ny == m_images[3]->ny() )
            {
                for( uint64_t n = 0; n < m_nx * m_ny; ++n )
                {
                    if( m_images[3]->pixel( n ) > 0 && m_satData[n] == 0 )
                    {
                        m_satData[n] = 1;
                        ++m_saturated;
                    }
                    // don't set 0 b/c it would override m_satLevel
                }
            }
        }

        // copy out mask to m_mask if valid

    } // unique lock scope -- lock and rawlock release

    { // shared lock scope

        sharedLockT lock( m_calMutex );
        RTIMV_DEBUG_BREADCRUMB

        // If we're here with the lock and qim hasn't been set bail now
        if( !m_qim )
        {
            return;
        }

        RTIMV_DEBUG_BREADCRUMB

        // At this point the raw data has been copied out to calData.
        // We have released the raw data mutex (rawlock).
        // We shared_lock the caldata mutex to make sure a subsequent call
        // from a different thread doesn't delete m_calData.

        RTIMV_DEBUG_BREADCRUMB

        m_minImageData = std::numeric_limits<float>::max();
        m_maxImageData = -std::numeric_limits<float>::max();

        float imval;

        // get mask function that is just 1 if not valid, otherwise goes into m_mask

        for( uint32_t j = 0; j < m_ny; ++j )
        {
            for( uint32_t i = 0; i < m_nx; ++i )
            {
                imval = calPixel( i, j );

                if( !std::isfinite( imval ) )
                {
                    continue;
                }

                if( imval > m_maxImageData )
                {
                    m_maxImageData = imval;
                }

                if( imval < m_minImageData )
                {
                    m_minImageData = imval;
                }
            }
        }

        RTIMV_DEBUG_BREADCRUMB

        if( !std::isfinite( m_maxImageData ) || !std::isfinite( m_minImageData ) )
        {
            // It should be impossible for them to be infinite by themselves unless it's all NaNs.
            m_maxImageData = 0;
            m_minImageData = 0;
        }

        if( ( resized || m_autoScale ) && m_colormode != rtimv::colormode::minmaxbox )
        {
            minScaleData( m_minImageData );
            maxScaleData( m_maxImageData );
        }
        else if( m_colormode == rtimv::colormode::minmaxbox )
        {
            normalizeColorBox();

            m_colorBox_min = std::numeric_limits<float>::max();
            m_colorBox_max = -std::numeric_limits<float>::max();

            float imval;

            for( uint32_t j = m_colorBox_j0; j <= m_colorBox_j1; ++j )
            {
                for( uint32_t i = m_colorBox_i0; i <= m_colorBox_i1; ++i )
                {
                    imval = calPixel( i, j );

                    /*if(valid(2))
                    {
                       if(_pixel_mask(i,j)) // no get a mask function, that just returns 1 if not valid
                       {
                          continue;
                       }
                    }
                    */
                    if( !std::isfinite( imval ) )
                    {
                        continue;
                    }

                    if( imval < m_colorBox_min )
                    {
                        m_colorBox_min = imval;
                    }
                    if( imval > m_colorBox_max )
                    {
                        m_colorBox_max = imval;
                    }
                }
            }

            if( !std::isfinite( m_colorBox_max ) || !std::isfinite( m_colorBox_min ) )
            {
                // It should be impossible for them to be infinite by themselves unless it's all NaNs in the box.
                m_colorBox_max = 0;
                m_colorBox_min = 0;
            }

            if( resized || m_autoScale )
            {
                minScaleData( m_colorBox_min );
                maxScaleData( m_colorBox_max );
            }
        }

        RTIMV_DEBUG_BREADCRUMB

        mtxL_recolorImpl();

        RTIMV_DEBUG_BREADCRUMB

        mtxL_postRecolor( lock );

        RTIMV_DEBUG_BREADCRUMB

        mtxL_postChangeImdata( lock );

        RTIMV_DEBUG_BREADCRUMB

        if( resized )
        {
            // Always switch to zoom 1 after a resize occurs
            zoomLevel( 1 );
        }

        RTIMV_DEBUG_BREADCRUMB

    } // shared mutex scope. - at this point we're done with calData

    RTIMV_DEBUG_BREADCRUMB

} // void rtimvBase::mtxUL_changeImdata()

void rtimvBase::mtxL_recolorImpl()
{
    /* Here is where we color the qImage*/

    RTIMV_DEBUG_BREADCRUMB

    if( !m_qim )
    {
        return;
    }

    RTIMV_DEBUG_BREADCRUMB

    // Get the color index calculating function
    int ( *_index )( float, float, float, int, int );
    switch( m_stretch )
    {
    case rtimv::stretch::log:
        _index = calcPixIndex_log;
        break;
    case rtimv::stretch::pow:
        _index = calcPixIndex_pow;
        break;
    case rtimv::stretch::sqrt:
        _index = calcPixIndex_sqrt;
        break;
    case rtimv::stretch::square:
        _index = calcPixIndex_square;
        break;
    default:
        _index = calcPixIndex_linear;
    }

    RTIMV_DEBUG_BREADCRUMB

    if( m_minScaleData == m_maxScaleData )
    {
        float imval;
        for( uint32_t i = 0; i < m_ny; ++i )
        {
            for( uint32_t j = 0; j < m_nx; ++j )
            {
                imval = calPixel( j, i );
                if( !std::isfinite( imval ) )
                {
                    m_qim->setPixel( j, m_ny - i - 1, m_nanColor );
                    continue;
                }

                m_qim->setPixel( j, m_ny - i - 1, 0 );
            }
        }
    }
    else
    {
        float imval;
        for( uint32_t i = 0; i < m_ny; ++i )
        {
            for( uint32_t j = 0; j < m_nx; ++j )
            {
                imval = calPixel( j, i );

                if( !std::isfinite( imval ) )
                {
                    m_qim->setPixel( j, m_ny - i - 1, m_nanColor );
                    continue;
                }

                int idxVal = _index( imval, m_minScaleData, m_maxScaleData, m_minColor, m_maxColor );
                m_qim->setPixel( j, m_ny - i - 1, idxVal );
            }
        }
    }

    RTIMV_DEBUG_BREADCRUMB

    if( m_applySatMask )
    {
        for( uint32_t j = 0; j < m_ny; ++j )
        {
            for( uint32_t i = 0; i < m_nx; ++i )
            {
                if( satPixel( i, j ) == 1 )
                {
                    m_qim->setPixel( i, m_ny - j - 1, m_satColor );
                }
            }
        }
    }

    RTIMV_DEBUG_BREADCRUMB
}

void rtimvBase::mtxUL_recolor()
{
    sharedLockT lock( m_calMutex );
    mtxL_recolor( lock );
}

void rtimvBase::mtxL_recolor( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    // Check if we're currently changing image data, this means a recolor is about to happen
    std::unique_lock changeLock{ m_changingImData, std::defer_lock };

    bool amChanging = !changeLock.try_lock();

    if( amChanging )
    {
        return;
    }
    /*else
    {
        changeLock.unlock();
    }*/

    mtxL_recolorImpl();

    mtxL_postRecolor( lock );
}

float rtimvBase::minImageData()
{
    return m_minImageData;
}

float rtimvBase::maxImageData()
{
    return m_maxImageData;
}

void rtimvBase::zoomLevel( float zl )
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

void rtimvBase::normalizeColorBox()
{
    if( m_colorBox_i0 < 0 )
    {
        m_colorBox_i0 = 0;
    }
    else if( m_colorBox_i0 >= (int64_t)m_nx - 1 )
    {
        m_colorBox_i0 = (int64_t)m_nx - 2;
    }

    if( m_colorBox_i1 < 0 )
    {
        m_colorBox_i1 = 0;
    }
    else if( m_colorBox_i1 >= (int64_t)m_nx - 1 )
    {
        m_colorBox_i1 = m_nx - 2;
    }

    if( m_colorBox_i0 > m_colorBox_i1 )
    {
        std::swap( m_colorBox_i0, m_colorBox_i1 );
    }

    if( m_colorBox_j0 < 0 )
    {
        m_colorBox_j0 = 0;
    }
    else if( m_colorBox_j0 >= (int64_t)m_ny - 1 )
    {
        m_colorBox_j0 = (int64_t)m_ny - 2;
    }

    if( m_colorBox_j1 <= 0 )
    {
        m_colorBox_j1 = 0;
    }
    else if( m_colorBox_j1 >= (int64_t)m_ny - 1 )
    {
        m_colorBox_j1 = (int64_t)m_ny - 2;
    }

    if( m_colorBox_j0 > m_colorBox_j1 )
    {
        std::swap( m_colorBox_j0, m_colorBox_j1 );
    }
}

bool rtimvBase::realTimeStopped()
{
    return m_realTimeStopped;
}

void rtimvBase::realTimeStopped( bool rts )
{
    m_realTimeStopped = rts;

    if( m_realTimeStopped )
    {
        m_foundation->m_imageTimer.stop();
    }
    else
    {
        m_foundation->m_imageTimer.start( m_imageTimeout );
    }
}

void rtimvBase::updateFPS()
{
    return;
}

void rtimvBase::updateAge()
{
    return;
}

void rtimvBase::updateNC()
{
    return;
}
