#include "rtimvBase.hpp"

#include <utility>

#ifdef RTIMV_MILK
    #include "images/shmimImage.hpp"
#endif

#include "images/fitsImage.hpp"
#include "images/fitsDirectory.hpp"
#include "images/mzmqImage.hpp"

// #define RTIMV_DEBUG_BREADCRUMB std::cerr << __FILE__ << " " << __LINE__ << "\n";
#define RTIMV_DEBUG_BREADCRUMB

rtimvBase::rtimvBase( QWidget *Parent, Qt::WindowFlags f ) : QWidget( Parent, f )
{
}

rtimvBase::rtimvBase( const std::vector<std::string> &shkeys, QWidget *Parent, Qt::WindowFlags f )
    : QWidget( Parent, f )
{
    startup( shkeys );
}

void rtimvBase::startup( const std::vector<std::string> &shkeys )
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
                        isFits = true;
                }
                if( shkeys[i].size() > 5 && !isFits )
                {
                    if( shkeys[i].rfind( ".fits" ) == shkeys[i].size() - 5 ||
                        shkeys[i].rfind( ".FITS" ) == shkeys[i].size() - 5 )
                        isFits = true;
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
#ifdef RTIMV_MILK
                    // If we get here we try to interpret as an ImageStreamIO image
                    shmimImage *si = new shmimImage( &m_rawMutex );
                    m_images[i] = (rtimvImage *)si;
#else
                    qFatal( "Unrecognized image key format" );
#endif
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

    connect( &m_imageTimer, SIGNAL( timeout() ), this, SLOT( updateImages() ) );
    connect( &m_cubeTimer, SIGNAL( timeout() ), this, SLOT( updateCube() ) );
    connect( &m_cubeFrameUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateCubeFrame() ) );
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
    emit nzUpdated( m_nz );

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

        mtxL_load_colorbar( current_colorbar, false, lock ); // have to load into newly created image

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
        m_cubeTimer.stop();

        if( m_nz <= 1 )
        {
            m_cubeFrameUpdateTimer.stop();
        }
        else
        {
            m_cubeFrameUpdateTimer.start( 250 );
        }

        m_cubeFPS = 0;

        emit cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );

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
            m_cubeTimer.start( cubeTimeout );
            m_cubeFrameUpdateTimer.start( 250 );
        }
        else
        {
            m_cubeFPS = 0;
            m_cubeTimer.stop();
        }

        emit cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
    }

    if( currImageTimeout == m_currImageTimeout ) // Don't interrupt if not needed
    {
        return;
    }

    m_currImageTimeout = currImageTimeout;

    m_imageTimer.stop();

    for( size_t i = 0; i < m_images.size(); ++i )
    {
        if( m_images[i] != nullptr )
        {
            m_images[i]->timeout( m_currImageTimeout ); // just for fps calculations
        }
    }

    m_imageTimer.start( m_currImageTimeout );
}

void rtimvBase::imageTimeout( int to )
{
    m_imageTimeout = to;

    setCurrImageTimeout();
}

void rtimvBase::cubeMode( bool cm )
{
    m_cubeMode = cm;

    setCurrImageTimeout();

    emit cubeModeUpdated( m_cubeMode );
}

void rtimvBase::cubeFPS( float fps )
{
    if( fps < 0 )
    {
        fps = 0;
    }
    m_desiredCubeFPS = fps;
    setCurrImageTimeout();

    emit cubeFPSUpdated( m_cubeFPS, m_desiredCubeFPS );
}

void rtimvBase::cubeFPSMult( float mult )
{
    m_cubeFPSMult = mult;
    setCurrImageTimeout();
    emit cubeFPSMultUpdated( m_cubeFPSMult );
}

void rtimvBase::cubeDir( int dir )
{
    m_cubeDir = dir;
    emit cubeDirUpdated( m_cubeDir );
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
        mtxUL_changeImdata( true );

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
    emit cubeFrameUpdated( m_images[0]->imageNo() );
}

int rtimvBase::imageTimeout()
{
    return m_imageTimeout;
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

// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color/56678483#56678483
template <typename realT>
realT sRGBtoLinRGB( int rgb )
{
    realT V = ( (realT)rgb ) / 255.0;

    if( V <= 0.0405 )
        return V / 12.92;

    return pow( ( V + 0.055 ) / 1.055, 2.4 );
}

template <typename realT>
realT linRGBtoLuminance( realT linR, realT linG, realT linB )
{
    return 0.2126 * linR + 0.7152 * linG + 0.0722 * linB;
}

template <typename realT>
realT pLightness( realT lum )
{
    if( lum <= static_cast<realT>( 216 ) / static_cast<realT>( 24389 ) )
    {
        return lum * static_cast<realT>( 24389 ) / static_cast<realT>( 27 );
    }

    return pow( lum, static_cast<realT>( 1 ) / static_cast<realT>( 3 ) ) * 116 - 16;
}

void rtimvBase::set_cbStretch( int ct )
{
    if( ct < 0 || ct >= cbStretches_max )
    {
        ct = stretchLinear;
    }

    m_cbStretch = ct;
}

int rtimvBase::get_cbStretch()
{
    return m_cbStretch;
}

void rtimvBase::mindat( float md )
{
    m_mindat = md;
}

float rtimvBase::mindat()
{
    return m_mindat;
}

void rtimvBase::maxdat( float md )
{
    m_maxdat = md;
}

float rtimvBase::maxdat()
{
    return m_maxdat;
}

void rtimvBase::bias( float b )
{
    float cont = contrast();

    mindat( b - 0.5 * cont );
    maxdat( b + 0.5 * cont );
}

float rtimvBase::bias()
{
    return 0.5 * ( m_maxdat + m_mindat );
}

void rtimvBase::bias_rel( float br )
{
    float cont = contrast();

    mindat( imdat_min + br * ( imdat_max - imdat_min ) - 0.5 * cont );
    maxdat( imdat_min + br * ( imdat_max - imdat_min ) + 0.5 * cont );
}

float rtimvBase::bias_rel()
{
    return 0.5 * ( m_maxdat + m_mindat ) / ( m_maxdat - m_mindat );
}

void rtimvBase::contrast( float c )
{
    float b = bias();
    mindat( b - 0.5 * c );
    maxdat( b + 0.5 * c );
}

float rtimvBase::contrast()
{
    return m_maxdat - m_mindat;
}

float rtimvBase::contrast_rel()
{
    return ( imdat_max - imdat_min ) / ( m_maxdat - m_mindat );
}

void rtimvBase::contrast_rel( float cr )
{
    float b = bias();
    mindat( b - .5 * ( imdat_max - imdat_min ) / cr );
    maxdat( b + .5 * ( imdat_max - imdat_min ) / cr );
}

int calcPixIndex_linear( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );
    if( pixval < 0 )
        return 0;

    // Clamp it to <= 1
    if( pixval > 1. )
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * ( maxcol - mincol ) + 0.5;
}

int calcPixIndex_log( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    static float a = 1000;
    static float log10_a = log10( a );

    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );
    if( pixval < 0 )
        return 0;

    pixval = log10( pixval * a + 1 ) / log10_a;

    // Clamp it to <= 1
    if( pixval > 1. )
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * ( maxcol - mincol ) + 0.5;
}

int calcPixIndex_pow( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    static float a = 1000;

    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );
    if( pixval < 0 )
        return 0;

    pixval = ( pow( a, pixval ) ) / a;

    // Clamp it to <= 1
    if( pixval > 1. )
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * ( maxcol - mincol ) + 0.5;
}

int calcPixIndex_sqrt( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );
    if( pixval < 0 )
        return 0;

    pixval = sqrt( pixval );

    // Clamp it to <= 1
    if( pixval > 1. )
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * ( maxcol - mincol ) + 0.5;
}

int calcPixIndex_square( float pixval, float mindat, float maxdat, int mincol, int maxcol )
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = ( pixval - mindat ) / ( (float)( maxdat - mindat ) );
    if( pixval < 0 )
        return 0;

    pixval = pixval * pixval;

    // Clamp it to <= 1
    if( pixval > 1. )
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * ( maxcol - mincol ) + 0.5;
}

void rtimvBase::mtxUL_changeImdata( bool newdata )
{
    RTIMV_DEBUG_BREADCRUMB

    if( m_amChangingimdata && !newdata ) // this means we're already in this function!
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
        if( !imageValid( 0 ) || ( m_amChangingimdata && !newdata ) )
        {
            return;
        }

        m_amChangingimdata = true;

        // Here we realize we need to resize
        if( m_images[0]->nx() != m_nx || m_images[0]->ny() != m_ny || !m_qim )
        {
            RTIMV_DEBUG_BREADCRUMB

            mtxL_setImsize( m_images[0]->nx(), m_images[0]->ny(), m_images[0]->nz(), lock );

            RTIMV_DEBUG_BREADCRUMB

            resized = true;
        }

        RTIMV_DEBUG_BREADCRUMB

        // If it's new data we copy it to m_calData
        if( resized || newdata )
        {
            RTIMV_DEBUG_BREADCRUMB

            // Get the pixel calculating function
            float ( *_pixel )( rtimvBase *, size_t ) = rawPixel();

            if( _pixel == nullptr )
            {
                m_amChangingimdata = false;

                return;
            }

            RTIMV_DEBUG_BREADCRUMB

            if( m_nx != m_images[0]->nx() || m_ny != m_images[0]->ny() )
            {
                m_amChangingimdata = false;
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
        }

    } // mutex scope -- lock and rawlock release
    { // mutex scope

        sharedLockT lock( m_calMutex );
        RTIMV_DEBUG_BREADCRUMB

        // At this point the raw data has been copied out to calData.
        // We have released the raw data mutex (rawlock).
        // We shared_lock the caldata mutex to make sure a subsequent call
        // from a different thread doesn't delete m_calData.
        // This could be forced by newdata

        RTIMV_DEBUG_BREADCRUMB

        imdat_min = std::numeric_limits<float>::max();
        imdat_max = -std::numeric_limits<float>::max();

        float imval;

        for( uint32_t j = 0; j < m_ny; ++j )
        {
            for( uint32_t i = 0; i < m_nx; ++i )
            {
                imval = calPixel( i, j );

                if( !std::isfinite( imval ) )
                {
                    continue;
                }

                if( imval > imdat_max )
                {
                    imdat_max = imval;
                }

                if( imval < imdat_min )
                {
                    imdat_min = imval;
                }
            }
        }

        RTIMV_DEBUG_BREADCRUMB

        if( !std::isfinite( imdat_max ) || !std::isfinite( imdat_min ) )
        {
            // It should be impossible for them to be infinite by themselves unless it's all NaNs.
            imdat_max = 0;
            imdat_min = 0;
        }

        if( ( resized || ( newdata && m_autoScale ) ) && !colorBoxActive )
        {
            mindat( imdat_min );
            maxdat( imdat_max );
        }
        else if( colorBoxActive )
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

            if( resized || ( newdata && m_autoScale ) )
            {
                mindat( m_colorBox_min );
                maxdat( m_colorBox_max );
            }
        }

        RTIMV_DEBUG_BREADCRUMB

        if( !m_qim )
        {
            m_amChangingimdata = false;
            return;
        }

        RTIMV_DEBUG_BREADCRUMB

        mtxL_recolor( lock );

        mtxL_postChangeImdata( lock );

        if( resized )
        {
            // Always switch to zoom 1 after a resize occurs
            zoomLevel( 1 );
        }

        RTIMV_DEBUG_BREADCRUMB

    } // mutex scope. - at this point we're done with calData

    RTIMV_DEBUG_BREADCRUMB

    m_amChangingimdata = false;

} // void rtimvBase::mtxUL_changeImdata(bool newdata)

void rtimvBase::mtxL_recolor()
{
    /* Here is where we color the pixmap*/

    // Get the color index calculating function
    int ( *_index )( float, float, float, int, int );
    switch( m_cbStretch )
    {
    case stretchLog:
        _index = calcPixIndex_log;
        break;
    case stretchPow:
        _index = calcPixIndex_pow;
        break;
    case stretchSqrt:
        _index = calcPixIndex_sqrt;
        break;
    case stretchSquare:
        _index = calcPixIndex_square;
        break;
    default:
        _index = calcPixIndex_linear;
    }

    if( m_mindat == m_maxdat )
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

                int idxVal = _index( imval, m_mindat, m_maxdat, m_minColor, m_maxColor );
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

    m_qpm.convertFromImage( *m_qim, Qt::AutoColor | Qt::ThresholdDither );
}

void rtimvBase::mtxL_recolor( const uniqueLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_recolor();

    mtxL_postRecolor( lock );
}

void rtimvBase::mtxL_recolor( const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    mtxL_recolor();

    mtxL_postRecolor( lock );
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

void rtimvBase::colorBox_i0( int64_t i0 )
{
    m_colorBox_i0 = i0;
}

int64_t rtimvBase::colorBox_i0()
{
    return m_colorBox_i0;
}

void rtimvBase::colorBox_i1( int64_t i1 )
{
    m_colorBox_i1 = i1;
}

int64_t rtimvBase::colorBox_i1()
{
    return m_colorBox_i1;
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

int64_t rtimvBase::colorBox_j1()
{
    return m_colorBox_j1;
}

void rtimvBase::mtxL_setColorBoxActive( bool usba, const sharedLockT &lock )
{
    assert( lock.owns_lock() );

    if( usba )
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
                    m_colorBox_min = imval;
                if( imval > m_colorBox_max )
                    m_colorBox_max = imval;
            }
        }

        if( m_colorBox_min == std::numeric_limits<float>::max() &&
            m_colorBox_max == -std::numeric_limits<float>::max() ) // If all nans
        {
            m_colorBox_min = 0;
            m_colorBox_max = 0;
        }

        mindat( m_colorBox_min );
        maxdat( m_colorBox_max );

        set_colorbar_mode( minmaxbox );
    }
    else
    {
        set_colorbar_mode( minmaxglobal );
    }

    colorBoxActive = usba;

    mtxL_recolor( lock );

    mtxL_postSetColorBoxActive( usba, lock );
}

void rtimvBase::set_RealTimeEnabled( int rte )
{
    RealTimeEnabled = ( rte != 0 );
}

void rtimvBase::set_RealTimeStopped( int rts )
{
    RealTimeStopped = ( rts != 0 );

    if( RealTimeStopped )
    {
        m_imageTimer.stop();
    }
    else
    {
        m_imageTimer.start( m_imageTimeout );
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
