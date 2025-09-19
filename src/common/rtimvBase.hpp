/** \file rtimvBase.hpp
 * \brief Declarations for the rtimvBase base class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvBase_hpp
#define rtimv_rtimvBase_hpp

#include <vector>
#include <mutex>
#include <shared_mutex>

#include <QImage>

#include <mx/app/application.hpp>

#include "rtimvImage.hpp"
#include "colorMaps.hpp"

#include "rtimvBaseObject.hpp"

/// The base class for rtimv functions
/** Manages access to images based on specified format and protocol.  On each image update this
 * colors the image according to the current configuration.
 *
 * Access to calibrated image data is protected by \ref m_calMutex, which is a protected member.
 * Functions which depend on/require the mutex to be in a certain state are prefixed with
 * - `mtxL_` if the mutex should be locked
 * - `mtxUL_` if the mutex should be unlocked.
 * Such functions often also take the mutex as an argument and verify that the mutex is in the correct state.
 * Calling an `mtxL_` function with the mutex unlocked will result in concurrency bugs and crashes. Attempting to
 * lock \ref m_calMutex from within a call to such a function will result in a deadlock.
 * Note that a `mtxUL_` function likely locks the mutex, so calling it with the mutex locked may
 * result in a deadlock.
 *
 * There is also \ref m_rawMutex which protects access to the raw image data.  This normally will not need to be
 * used by derived classes.
 *
 * This class is pure virtual.  The following virtual functions must be implemented in derived classes:
 * - \ref virtual void mtxL_postSetImsize( const uniqueLockT &lock )
 * - \ref virtual void mtxL_postRecolor( const uniqueLockT &lock )
 * - \ref virtual void mtxL_postRecolor( const sharedLockT &lock )
 * - \ref virtual void mtxL_postChangeImdata( const sharedLockT &lock )
 * - \ref virtual void post_zoomLevel()
 * - \ref virtual void mtxL_postSetColorBoxActive( bool usba, const sharedLockT &lock )
 *
 * Additional optional virtual functions that may be implemented are:
 * - \ref virtual void onConnect()
 * - \ref virtual void updateFPS();
 * - \ref virtual void updateAge();
 * - \ref virtual void updateNC();
 */
class rtimvBase : public mx::app::application
{

  public:
    typedef std::unique_lock<std::shared_mutex> uniqueLockT;

    typedef std::shared_lock<std::shared_mutex> sharedLockT;

  public:
    /** @name Construction
     *
     * @{
     */

    /// Basic c'tor.  Does not startup the images.
    /** startup should be called with the list of keys.
     */
    rtimvBase( );

    /// Image c'tor, starts up the images.
    /** startup should not be called.
     */
    rtimvBase( const std::vector<std::string> &shkeys ///< [in] The shmim keys used to access the images.
                );

    /// @}

    /** @name Configuration
     *
     *  The mx::app::application interface for command line and config files.
     * @{
     */
    virtual void setupConfig();

    virtual void loadConfig();

    ///@}

    rtimvBaseObject *m_foundation{ nullptr };

    /** @name Connection Data
     *
     * @{
     */

  protected:
    /// The images to be displayed
    /** By index:
     * - 0 is the main image.
     * - 1 is the dark image which is (optionally) subtracted from the main image.
     * - 2 is the mask image which is (optionally) multiplied by the dark-subtracted image.  Normally a 1/0 image.
     * - 3 is the saturation mask which (optionally) denotes which pixels to turn the saturation color.
     */
    std::vector<rtimvImage *> m_images;

    /// Flag used to indicate that the main image, m_images[0], is connected to its first image
    bool m_connected{ false };

    /// Flag to indicate that the milkzmq protocol should be used for all ImageStremIO images
    bool m_mzmqAlways{ false };

    /// Default milkzmq server to use, if set this overrides default "localhost" in \ref mzmqImage
    std::string m_mzmqServer;

    /// Default milkzmq server to use, if set this overrides default "5556" in \ref mzmqImage
    int m_mzmqPort{ 0 };

    ///@}

  public:
    /** @name Connection
     *
     * @{
     */

    /// Configure the image sources and start checking for updates.
    /**
     */
    void startup( const std::vector<std::string> &shkeys /**< [in] The keys used to access
                                                         the images (see \ref m_images)*/ );

    /// Function called on connection
    /**
     * This function must set m_connected to true if successful.
     */
    virtual void onConnect()
    {
        m_connected = true;
    }

    /// Check if an image is currently valid.
    /** An image is valid if it was supplied on command line, and if the image itself returns true from valid().
     *
     * \returns true if valid
     * \returns false otherwise
     */
    bool imageValid( size_t n /**< [in] the image number */ );

    ///@}

    /** @name Image Size Data
     *
     * @{
     */
  protected:
    uint32_t m_nx{ 0 }; ///< The number of pixels in the x (horizontal) direction

    uint32_t m_ny{ 0 }; ///< The number of pixels in the y (vertical) direction

    uint32_t m_nz{ 1 }; ///< The number of images in the cube.  Always >= 1.

    ///@}

  public:
    /** @name Image Size
     *
     * @{
     */

    /// Changes the image size, but only if necessary.
    /** This reallocates m_calData and m_qim
     *
     */
    void mtxL_setImsize( uint32_t x, ///< [in] the new x size
                         uint32_t y, ///< [in] the new y size
                         uint32_t z, ///< [in] the new z size
                         const uniqueLockT &lock );

    /// Called after set_imsize to handle allocations for derived classes
    /**
     *
     * Called with m_calMutex in a unique lock.  Implementation should verify that the mutex is locked
     * with, e.g.
     * \code
     * assert( lock.owns_lock() );
     * \endcode
     */
    virtual void mtxL_postSetImsize( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ ) = 0;

    /// Get the number of x pixels
    /**
     * \returns the current value of m_nx
     */
    uint32_t nx();

    /// Get the number of y pixels
    /**
     * \returns the current vvalue of m_ny
     */
    uint32_t ny();

    /// Get the number of images
    /**
     * \returns the current vvalue of m_nz
     */
    uint32_t nz();

    /// @}

    /** @name Image Update - Data
     *
     * @{
     */

  protected:
    int m_imageTimeout{ 50 }; ///< The minimum timeout for checking for a new images, ms.

    bool m_cubeMode{ false }; ///< Whether or not cube mode is enabled.

    float m_cubeFPS{ 20 }; ///< The cube frame rate

    float m_desiredCubeFPS{ 20 }; ///< The cube frame rate

    float m_cubeFPSMult{ 1.0 }; ///< Multiplier on cube FPS, e.g. for fast forwarding

    int m_cubeDir{ 1 }; ///< Direction of cube travel. +1 or -1.

    int m_currImageTimeout{ 50 }; /**< The timeout for checking for a new image in ms.  This is
                                       what is used to maintain both the cube update rate set by
                                       m_cubeFPS (as close as possible) while meeting the minimum
                                       interval set by m_imageTimeout. */

    ///@}

    /** @name Image Update - Slots and Signals
     *
     * @{
     */
  public:
    /// Set the image display timeout.
    /** This sets the display frame rate, e.g. a timeout of 50 msec will
     * cause the display to update at 20 f.p.s. (the default setting).
     */
    void imageTimeout( int to /**< [in] the new image display timeout*/ );

    void cubeMode( bool cm /**< [in] */ );

    void cubeFPS( float fps /**< [in] */ );

    void cubeFPSMult( float mult /**< [in] */ );

    void cubeDir( int dir /**< [in] */ );

    void cubeFrame( uint32_t fno /**< [in] */ );

    void cubeFrameDelta( int32_t dfno /**< [in] the change in image number */ );

    /// Check all images for updates
    /** This is called on m_imageTimer expiration.
     */
    void updateImages();

    /// Increment the main image cube number
    /** This is on m_cubeTimer expiration
     */
    void updateCube();

    /// Update the cube frame number
    /**
     *  Emits cubeFrameUpdated(uint32_t)
     */
    void updateCubeFrame();

    ///@}

    /** @name Image Update Member Access
     *
     * @{
     */

  private:
    void setCurrImageTimeout();

  public:
    /// Get the minimum image display timeout.
    /**
     * \returns the current value of m_imageTimeout
     */
    int imageTimeout();

    /// Get the current image display timeout.
    /**
     * \returns the current value of m_currImageTimeout
     */
    int currImageTimeout();

    /// @}

    /** \name Calibrated Pixel Data
     *
     * Settings to control which calibrations are applied, and manage access to memory.
     *
     * @{
     */

  protected:
    /// Whether or not the dark image is subtracted, default is false.
    bool m_subtractDark{ false };

    /// Whether or not the mask is applied, default is false.
    bool m_applyMask{ false };

    /// Whether or not the saturation mask is applied, default is false.
    /** Note this only controls whether the pixels are colored m_satColor.  It does
     * not change the values returned by rawPixel().
     */
    bool m_applySatMask{ false };

    float *m_calData{ nullptr };
    uint8_t *m_satData{ nullptr };

    /// Mutex for locking access to raw pixels
    /** This is used by rtimvImage derived classes to protect
     * deletion and recreation of the m_data array they manage.
     *
     */
    std::mutex m_rawMutex;

    /// Mutex for locking access to calibrated pixels
    /** Most uses require non-exclusive shared-locking for readings
     */
    std::shared_mutex m_calMutex;

    ///@}

    /** \name Calibrated Pixel Access
     *
     * Functions to manage which calibrations are applied and provide access to calibrated pixels.
     *
     * Calibrations include dark subtraction, reference subtraction, flat field, mask, and low and high pass filtering.
     * Note: only dark subtraction and masking are currently implemented.
     *
     * The pixelF function pointer is used so that only a single `if-else` tree needs to be evaluated before
     * iterating over all pixels.  The rawPixel() function returns a pointer to the static pixel_* function appropriate
     * for the current calibration settings.
     *
     * @{
     */
  public:
    /// The fuction pointer type for accessing pixels with calibrations applied.
    typedef float ( *pixelF )( rtimvBase *, size_t );

    /// Returns a pointer to the static pixel value calculation function for the current calibration configuration
    /** Calibration configuration includes the value of m_subtractDark, m_applyMask.
     *
     * \returns a pointer to one of pixel_noCal, pixel_subDark, pixel_applyMask, pixel_subDarkApplyMask.
     */
    pixelF rawPixel();

    /// Access pixels with no calibrations applied.
    /**
     * \returns the value of pixel idx
     */
    static float pixel_noCal( rtimvBase *imv, ///< [in] the rtimvBase instance to access
                              size_t idx      ///< [in] the linear pixel number to access
    );

    /// Access pixels with dark subtraction applied.
    /**
     * \returns the value of pixel idx after dark subtraction
     */
    static float pixel_subDark( rtimvBase *imv, ///< [in] the rtimvBase instance to access
                                size_t idx      ///< [in] the linear pixel number to access
    );

    /// Access pixels with the mask applied.
    /**
     * \returns the value of pixel idx after applying the mask
     */
    static float pixel_applyMask( rtimvBase *imv, ///< [in] the rtimvBase instance to access
                                  size_t idx      ///< [in] the linear pixel number to access
    );

    /// Access pixels with dark subtraction and masking applied.
    /**
     * \returns the value of pixel idx after subtracting the dark and applying the mask
     */
    static float pixel_subDarkApplyMask( rtimvBase *imv, ///< [in] the rtimvBase instance to access
                                         size_t idx      ///< [in] the linear pixel number to access
    );

    /// Get the value of a calibrated pixel
    /**
     * \returns the value of the (x,y) pixel in \ref m_calData
     */
    float calPixel( uint32_t x, /**< [in] the x coordinate of the pixel */
                    uint32_t y /**< [in] the y coordinate of the pixel */ );

    /// Get the value of a pixel in the saturation mask
    /**
     * \returns the value of the (x,y) pixel in \ref m_satData
     */
    uint8_t satPixel( uint32_t x, /**< [in] the x coordinate of the pixel */
                      uint32_t y /**< [in] the y coordinate of the pixel */ );

    ///@}

    /** @name Colorbar Selection
     *
     * @{
     */

  public:
    typedef int ( *pixelIndexF )( float );

    enum en_cbStretches
    {
        stretchLinear, ///< The pixel values are scaled linearly to between m_mindat and m_maxdat
        stretchLog,    ///< The pixel values are scaled logarithmically between m_mindat and m_maxdat
        stretchPow,    ///< the pixel values are scaled as \f$ 1000^p/1000 \f$ between m_mindat and m_maxdat
        stretchSqrt,   ///< the pixel values are scaled as \f$ \sqrt(p) \f$ between m_mindat and m_maxdat
        stretchSquare, ///< the pixel values are scaled as \f$ p^2 \f$ between m_mindat and m_maxdat
        cbStretches_max
    };

  protected:
    int m_minColor{ 0 }; ///< The minimum index to use for the color table.

    int m_maxColor{ 253 }; ///< The maximum index to use for the color table.

    int m_maskColor{ 254 }; ///< The index in the color table to use for the mask color.

    int m_satColor{ 255 }; ///< The index in the color table to use for the saturation color.

    int m_nanColor{ 254 }; ///< The index in the color table to use for nans and infinities.

    int colorbar_mode{ minmaxglobal };
    int m_cbStretch{ stretchLinear };

    int current_colorbar{ colorbarBone };

    QColor warning_color;

  public:
    enum colorbars
    {
        colorbarGrey,
        colorbarJet,
        colorbarHot,
        colorbarBone,
        colorbarRed,
        colorbarGreen,
        colorbarBlue,
        colorbarMax
    };

    template <typename lockT>
    void mtxL_load_colorbar( int cb, bool update, const lockT &lock );

    int get_current_colorbar()
    {
        return current_colorbar;
    }

    enum colorbar_modes
    {
        minmaxglobal,
        minmaxbox,
        user,
        colorbar_modes_max
    };

    void set_colorbar_mode( int mode )
    {
        colorbar_mode = mode;
    }

    int get_colorbar_mode()
    {
        return colorbar_mode;
    }

    void set_cbStretch( int );
    int get_cbStretch();

    pixelIndexF pixelIndex();

    static int pixelIndex_linear( float d );

    static int pixelIndex_log( float d );

    static int pixelIndex_pow( float d );

    static int pixelIndex_sqrt( float d );

    static int pixelIndex_square( float d );

    ///@}

    /** @name Colorbar Scale Control
     *
     * @{
     */

  protected:
    /*** Color Map ***/
    float m_mindat; ///< The minimum data value used for scaling

    float m_maxdat; ///< The maximum data valuse used for scaling

    bool m_autoScale{ false };

  public:
    void mindat( float md );

    float mindat();

    void maxdat( float md );

    float maxdat();

    void bias( float b );

    float bias();

    void bias_rel( float b );

    float bias_rel();

    void contrast( float c );

    float contrast();

    void contrast_rel( float cr );

    float contrast_rel();

    ///@}

    /** @name Image Filtering
     *
     * @{
     */

    float *m_lowPassFiltered{ nullptr };

    bool m_applyLPFilter;

    int m_lpFilterType;

    ///@} -- filtering

    //****** The display *************
  protected:
    QImage *m_qim{ nullptr }; ///< A QT image, used to store the color-map encoded data

    std::vector<double> m_lightness; ///< The perceived lightness values of the colormap RGB values

    /// Flag indicating that changeImdata(bool) is currently executing
    bool m_amChangingimdata{ false };

  public:
    /// Updates the QImage and basic statistics after a new image.
    /** \param newdata determines whether statistics are calculated (true) or not (false).
     */
    void mtxUL_changeImdata( bool newdata = false );

  private:
    /// Color the image based on the current colormap configuration and stretch
    /** This uses the specified color scale (linear, log, etc), and the
     * specified stretch.  If all pixels are equal they are all set to 0.
     * Sets NaN pixels to the \ref m_nanColor.
     *
     * Also sets any saturated pixels to \ref m_satColor.
     *
     */
    void mtxL_recolor();

  public:
    /// Perform a recolor when \ref m_calMutex is in unique lock
    /**
     * Calls \ref mtxL_recolor()
     *
     */
    void mtxL_recolor( const uniqueLockT &lock );

    /// Perform a recolor when \ref m_calMutex is in shared lock
    /**
     * Calls \ref mtxL_recolor()
     *
     */
    void mtxL_recolor( const sharedLockT &lock );

    /// Interface for derived class to perform actions after recolor.
    /** This is where the derived class updates the display.
     *
     * Called with m_calMutex in a unique lock.  Implementation should verify that the mutex is locked
     * with, e.g.
     * \code
     * assert( lock.owns_lock() );
     * \endcode
     *
     * Note that the two version of this differ only in the state of the mutex lock, which is necessary due to
     * the different circumstances under which an image is recolored.  Derived classes
     * will normally want to implement a single function using a template.
     */
    virtual void mtxL_postRecolor( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ ) = 0;

    /// Interface for derived class to perform actions after recolor.
    /** This is where the derived class updates the display.
     *
     * Called with m_calMutex in a shared lock.  Implementation should verify that the mutex is locked
     * with, e.g.
     * \code
     * assert( lock.owns_lock() );
     * \endcode
     *
     * Note that the two version of this differ only in the state of the mutex lock, which is necessary due to
     * the different circumstances under which an image is recolored.  Derived classes
     * will normally want to implement a single function using a template.
     *
     * \overload
     */
    virtual void
    mtxL_postRecolor( const sharedLockT &
                          lock /**<[in] a shared mutex lock which is locked*/ ) = 0; ///< to call after changing colors.

    /// Interface for derived classes to take any actions after the image data has changed
    /**
     * Called with m_calMutex in a shared lock.  Implementation should verify that the mutex is locked
     * with, e.g.
     * \code
     * assert( lock.owns_lock() );
     * \endcode
     *
     * \overload
     */
    virtual void mtxL_postChangeImdata( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ ) = 0;

  protected:
    float m_satLevel{ 1e30 };
    uint32_t m_saturated{ 0 };

    /* Image Stats */
  protected:
    float imdat_min;
    float imdat_max;

  public:
    float get_imdat_min()
    {
        return imdat_min;
    }
    float get_imdat_max()
    {
        return imdat_max;
    }

    /*** Abstract Zoom ***/
  protected:
    float m_zoomLevel{ 1 };
    float m_zoomLevelMin{ 1 };
    float m_zoomLevelMax{ 64 };

  public:
    float zoomLevel()
    {
        return m_zoomLevel;
    }
    float zoomLevelMin()
    {
        return m_zoomLevelMin;
    }
    float zoomLevelMax()
    {
        return m_zoomLevelMax;
    }

    // void set_ZoomLevel(int zlint);
    void zoomLevel( float zl );
    virtual void post_zoomLevel() = 0;

    /** @name A User Defined Region
     *
     * @{
     */
  protected:
    int colorBoxActive{ 0 };

  private:
    // ImageStreamIO images are sized in uint32_t, so we need these big enough for signed comparisons without wraparound
    int64_t m_colorBox_i0;
    int64_t m_colorBox_i1;
    int64_t m_colorBox_j0;
    int64_t m_colorBox_j1;

    void normalizeColorBox();

  protected:
    float m_colorBox_max;
    float m_colorBox_min;

  public:
    void colorBox_i0( int64_t i0 );
    int64_t colorBox_i0();

    void colorBox_i1( int64_t i1 );
    int64_t colorBox_i1();

    void colorBox_j0( int64_t j0 );
    int64_t colorBox_j0();

    void colorBox_j1( int64_t j1 );
    int64_t colorBox_j1();

    int getcolorBoxActive()
    {
        return colorBoxActive;
    }

    /// Change whether the color box is active
    void mtxL_setColorBoxActive( bool usba /**< [in] the new state of color box active */,
                                 const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    /// Take actions after the color box active state is changed
    virtual void
    mtxL_postSetColorBoxActive( bool usba /**< [in] the new state of color box active */,
                                const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ ) = 0;

    ///@}

    /*** Real Time Controls ***/
  protected:
    bool RealTimeEnabled{ true };  ///< Controls whether rtimvBase is using real-time data.
    bool RealTimeStopped{ false }; ///< Set when user temporarily freezes real-time data viewing.

  public:
    void set_RealTimeEnabled( int );
    void set_RealTimeStopped( int );

    virtual void updateFPS(); ///< Called whenever the displayed image updates its FPS.
    virtual void updateAge(); ///< Called whenever the displayed image updates its Age.
    virtual void updateNC();  ///< Update the display while not connected.
};

template <typename lockT>
void rtimvBase::mtxL_load_colorbar( int cb, bool update, const lockT &lock )
{
    assert( lock.owns_lock() );

    if( !m_qim )
    {
        return;
    }

    current_colorbar = cb;
    switch( cb )
    {
    case colorbarJet:
        m_minColor = 0;
        m_maxColor = load_colorbar_jet( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "white" );
        break;
    case colorbarHot:
        m_minColor = 0;
        m_maxColor = load_colorbar_hot( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "cyan" );
        break;
    case colorbarBone:
        m_minColor = 0;
        m_maxColor = load_colorbar_bone( m_qim );
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor( "red" );
        break;
    case colorbarRed:
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
    case colorbarGreen:
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
    case colorbarBlue:
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

    if( update )
    {
        mtxL_recolor( lock );
    }
}

#endif // rtimv_rtimvBase_hpp
