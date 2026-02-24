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
#include "rtimvColor.hpp"
#include "rtimvFilters.hpp"

#include "rtimvBaseObject.hpp"

#define RTIMV_BASE rtimvBase

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
    rtimvBase();

    ~rtimvBase();

    /// @}

    /** @name Configuration
     *
     *  The mx::app::application interface for command line and config files.
     * @{
     */
    virtual void setupConfig();

    virtual void loadConfig();

    /// Configure the image sources
    /**
     */
    void processKeys( const std::vector<std::string> &shkeys /**< [in] The keys used to access
                                                                       the images (see \ref m_images)*/ );

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

    /// Configured image names/keys by image index, used before streams are connected.
    std::vector<std::string> m_imageNames;

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

    /// Begin monitoring for updates
    void startup();

    /// Check if main image is currently connected to a source
    /**
     * \returns current value of m_connected
     */
    bool connected();

    /// Function called on connection
    /**
     * This function must set m_connected to true if successful.
     */
    virtual void onConnect();

    ///@}

    /** @name Image Status
     * @{
     */

    /// Check if the main image is currently valid.
    /** An image is valid if it was supplied on command line, and if the image itself returns true from valid().
     *
     * \returns true if valid
     * \returns false otherwise
     */
    bool imageValid();

    /// Check if an image is currently valid.
    /** An image is valid if it was supplied on command line, and if the image itself returns true from valid().
     *
     * \returns true if valid
     * \returns false otherwise
     */
    bool imageValid( size_t n /**< [in] the image number */ );

    /// Get the main image acquisition time.
    /**
     * \returns acquisition time of the main image (m_images[0]) if valid
     * \returns 0 if not valid
     */
    double imageTime();

    /// Get image acquisition time.
    /**
     * \returns acquisition time of the image if valid
     * \returns 0 if not valid
     */
    double imageTime( size_t n /**< [in] the image number */ );

    /// Get the main image FPS estimate.
    /**
     * \returns FPS estimate of the main image (m_images[0]) if valid
     * \returns 0 if not valid
     */
    double fpsEst();

    /// Get image FPS estimate.
    /**
     * \returns FPS estimate of the image if valid
     * \returns 0 if not valid
     */
    double fpsEst( size_t n /**< [in] the image number */ );

    /// Get the name of an image
    /**
     * \returns the name if valid
     * \returns an empty string if not valid
     */
    std::string imageName( size_t n /**< [in] the image number */ );

    /// Get the cube image number
    /**
     * \returns the image number if valid
     * \returns 0 if not valid
     */
    uint32_t imageNo( size_t n /**< [in] the image number */ );

    /// Get info for an image
    /**
     * \returns the info vector if valid
     * \returns and empty vector if not valid
     */
    std::vector<std::string> info( size_t n /**< [in] the image number */ );

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
    /** This reallocates \ref m_calDataRaw, updates \ref m_calData, and reallocates \ref m_qim.
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
     * \returns the current value of m_ny
     */
    uint32_t ny();

    /// Get the number of images
    /**
     * \returns the current value of m_nz
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

    float m_desiredCubeFPS{ 20 }; ///< The desired cube frame rate

    float m_cubeFPSMult{ 1.0 }; ///< Multiplier on cube FPS, e.g. for fast forwarding

    int m_cubeDir{ 1 }; ///< Direction of cube travel. +1 or -1.

    int m_currImageTimeout{ 50 }; /**< The timeout for checking for a new image in ms.  This is
                                       what is used to maintain both the cube update rate set by
                                       m_cubeFPS (as close as possible) while meeting the minimum
                                       interval set by m_imageTimeout. */

    ///@}

    /** @name Image Update Member Access
     *
     * @{
     */

  private:
    void setCurrImageTimeout();

  public:
    /// Set the image display timeout.
    /** This sets the maximum display frame rate, e.g. a timeout of 50 msec will
     * cause the display to update at 20 f.p.s. (the default setting).  The f.p.s.
     * will be slower if no new images are ready at that rate.
     */
    void imageTimeout( int to /**< [in] the new image display timeout*/ );

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

    /// Set the cube mode
    void cubeMode( bool cm /**< [in] the new cube mode*/ );

    /// Set the desired cube FPS
    void cubeFPS( float fps /**< [in] the new desired cube fps*/ );

    /// Set the cube FPS multiplier
    void cubeFPSMult( float mult /**< [in] the new cube FPS multiplier*/ );

    /// Set the cube direction
    /**
     * If negative the direction is backward.  Forward otherwise
     */
    void cubeDir( int dir /**< [in] the new cube direction*/ );

    /// Get the cube direction
    int cubeDir();

    /// Set the current cube frame
    void cubeFrame( uint32_t fno /**< [in] the new frame number*/ );

    /// Change the cube frame number by a delta
    void cubeFrameDelta( int32_t dfno /**< [in] the change in image number */ );

    /// @}

    /** @name Image Update - Slots
     *
     *  These aren't actually slots, but are the callbacks from the BaseObject's slots (m_foundation)
     *
     * @{
     */
  public:
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

    /// Owned buffer that stores unfiltered calibrated pixel data.
    float *m_calDataRaw{ nullptr };

    /// Pointer to the currently active calibrated data buffer.
    /** Normally this points at \ref m_calDataRaw. When filtering is enabled it points at
     * filter output buffers managed elsewhere.
     */
    float *m_calData{ nullptr };

    /// Buffer to hold the the saturated pixel map
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
    void subtractDark( bool sd );

    bool subtractDark();

    void applyMask( bool amsk );

    bool applyMask();

    void applySatMask( bool asmsk );

    bool applySatMask();

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

    /// Request a calibrated pixel value update.
    /** Local mode resolves this request immediately and emits
     * rtimvBaseObject::pixelValueUpdated.
     */
    void requestPixelValue( uint32_t x, /**< [in] the x coordinate of the pixel */
                            uint32_t y /**< [in] the y coordinate of the pixel */ );

    /// Get the value of a pixel in the saturation mask
    /**
     * \returns the value of the (x,y) pixel in \ref m_satData
     */
    uint8_t satPixel( uint32_t x, /**< [in] the x coordinate of the pixel */
                      uint32_t y /**< [in] the y coordinate of the pixel */ );

    ///@}

    /** @name Color Bar
     *
     * @{
     */

  protected:
    int m_minColor{ 0 }; ///< The minimum index to use for the color table.

    int m_maxColor{ 253 }; ///< The maximum index to use for the color table.

    int m_maskColor{ 254 }; ///< The index in the color table to use for the mask color.

    int m_satColor{ 255 }; ///< The index in the color table to use for the saturation color.

    int m_nanColor{ 254 }; ///< The index in the color table to use for nans and infinities.

    QColor warning_color;

    /// The current color bar
    rtimv::colorbar m_colorbar{ rtimv::colorbar::bone };

  private:
    /// Actual implementation of loading the color bar
    void mtxL_load_colorbarImpl( rtimv::colorbar cb /**< [in] the new color bar */ );

  public:
    /// Set the color bar
    /**
     * This loads the color bar specified and (optionally) updates the image
     */
    void mtxL_load_colorbar( rtimv::colorbar cb,     /**< [in] the new color bar */
                             bool update,            /**< [in] whether or not to update the image*/
                             const uniqueLockT &lock /**< [in] a lock on m_calMutex */
    );

    /// Set the color bar
    /**
     * This loads the color bar specified and (optionally) updates the image
     */
    void mtxL_load_colorbar( rtimv::colorbar cb,     /**< [in] the new color bar */
                             bool update,            /**< [in] whether or not to update the image*/
                             const sharedLockT &lock /**< [in] a lock on m_calMutex */
    );

    /// Set the color bar
    /**
     * This takes a shared lock on m_calMutex, then loads the color bar specified and (optionally) updates the image
     */
    void mtxUL_load_colorbar( rtimv::colorbar cb, /**< [in] the new color bar */
                              bool update         /**< [in] whether or not to update the image*/
    );

    rtimv::colorbar colorbar();

    ///@}

    /** @name Color Mode - data
     *
     * @{
     */

  protected:
    rtimv::colormode m_colormode{ rtimv::colormode::minmaxglobal };

    ///@}

    /** @name Color Mode
     *
     * The color mode defines what sets the min and max for the stretch.
     *
     * @{
     */
  public:
    /// Set the color mode
    /**
     * This version locks the m_calMutex. Then calls \ref mtxL_colormode, which results in a recolor.
     * If the mode is \ref rtimv::colorbar::minmaxbox this also
     * calculates the box min/max.
     *
     */
    void mtxUL_colormode( rtimv::colormode mode /**< [in] the new colormode */ );

    /// Set the color mode
    /**
     * This results in a recolor. If the mode is \ref rtimv::colorbar::minmaxbox this also
     * calculates the box min/max.
     *
     */
    void mtxL_colormode( rtimv::colormode mode, /**< [in] the new colormode */
                         const sharedLockT &lock /**<[in] a shared mutex lock which is locked on m_calMutex*/ );

    /// Get the current color mode
    rtimv::colormode colormode();

    /// Take actions after the color box active state is changed
    virtual void
    mtxL_postColormode( rtimv::colormode mode /**< [in] the new colormode */,
                        const sharedLockT &lock /**<[in] a shared mutex lock which is locked on m_calMutex*/ ) = 0;

    ///@}

    /** @name Color Box - Data
     *
     * @{
     */

  private:
    // ImageStreamIO images are sized in uint32_t, so these are big enough for signed comparisons without wraparound

    /// The color box upper left corner x coordinate
    int64_t m_colorBox_i0;

    /// The color box upper left corner y coordinate
    int64_t m_colorBox_i1;

    /// The color box lower right corner x coordinate
    int64_t m_colorBox_j0;

    /// The color box lower right corner y coordinate
    int64_t m_colorBox_j1;

    /// Adjusts the color box coordinates to ensure they are valid
    void normalizeColorBox();

  protected:
    /// The minimum calibrated value in the color box
    float m_colorBox_max;

    /// The maximum calibrated value in the color box
    float m_colorBox_min;

    ///@}

    /** @name Color Box
     *
     * @{
     */
  public:
    /// Set the color box upper left corner x coordinate
    void colorBox_i0( int64_t i0 /**< [in] the new  */ );

    /// Get the color box upper left corner x coordinate
    /**
     * \returns m_colorBox_i0
     */
    int64_t colorBox_i0();

    /// Set the color box upper left corner y coordinate
    void colorBox_j0( int64_t j0 /**< [in] the new  */ );

    /// Get the color box upper left corner y coordinate
    /**
     * \returns m_colorBox_j0
     */
    int64_t colorBox_j0();

    /// Set the color box lower right corner x coordinate
    void colorBox_i1( int64_t i1 /**< [in] the new  */ );

    /// Get the color box lower right corner x coordinate
    /**
     * \returns m_colorBox_i1
     */
    int64_t colorBox_i1();

    /// Set the color box lower right corner y coordinate
    void colorBox_j1( int64_t j1 /**< [in] the new  */ );

    /// Get the color box lower right corner y coordinate
    /**
     * \returns m_colorBox_j1
     */
    int64_t colorBox_j1();

    /// Get the minimum calibrated value in the color box
    /**
     * \returns m_colorBox_min
     */
    float colorBox_min();

    /// Get the maximum calibrated value in the color box
    /**
     * \returns m_colorBox_max
     */
    float colorBox_max();

    /// Request updated color-box min/max values.
    /** Local mode resolves this request immediately and emits
     * rtimvBaseObject::colorBoxUpdated.
     */
    void requestColorBoxValues();

    ///@}

    /** @name Stats Box - Data
     *
     * @{
     */
  private:
    /// The stats box upper left corner x coordinate
    int64_t m_statsBox_i0{ 0 };

    /// The stats box lower right corner x coordinate
    int64_t m_statsBox_i1{ 0 };

    /// The stats box upper left corner y coordinate
    int64_t m_statsBox_j0{ 0 };

    /// The stats box lower right corner y coordinate
    int64_t m_statsBox_j1{ 0 };

    /// Adjusts the stats box coordinates to ensure they are valid
    void normalizeStatsBox();

  protected:
    /// Whether stats-box calculations are enabled.
    bool m_statsBox{ false };

    /// The minimum calibrated value in the stats box.
    float m_statsBox_min{ 0 };

    /// The maximum calibrated value in the stats box.
    float m_statsBox_max{ 0 };

    /// The mean calibrated value in the stats box.
    float m_statsBox_mean{ 0 };

    /// The median calibrated value in the stats box.
    float m_statsBox_median{ 0 };

    /// Working storage used for median calculation.
    std::vector<float> m_statsBox_medWork;
    ///@}

    /** @name Stats Box
     *
     * @{
     */
  public:
    /// Set whether stats-box calculations are enabled.
    void statsBox( bool sb /**< [in] true enables stats-box calculations */ );

    /// Get whether stats-box calculations are enabled.
    bool statsBox();

    /// Set the stats box upper left corner x coordinate.
    void statsBox_i0( int64_t i0 /**< [in] the new upper-left x coordinate */ );

    /// Get the stats box upper left corner x coordinate.
    /**
     * \returns m_statsBox_i0
     */
    int64_t statsBox_i0();

    /// Set the stats box upper left corner y coordinate.
    void statsBox_j0( int64_t j0 /**< [in] the new upper-left y coordinate */ );

    /// Get the stats box upper left corner y coordinate.
    /**
     * \returns m_statsBox_j0
     */
    int64_t statsBox_j0();

    /// Set the stats box lower right corner x coordinate.
    void statsBox_i1( int64_t i1 /**< [in] the new lower-right x coordinate */ );

    /// Get the stats box lower right corner x coordinate.
    /**
     * \returns m_statsBox_i1
     */
    int64_t statsBox_i1();

    /// Set the stats box lower right corner y coordinate.
    void statsBox_j1( int64_t j1 /**< [in] the new lower-right y coordinate */ );

    /// Get the stats box lower right corner y coordinate.
    /**
     * \returns m_statsBox_j1
     */
    int64_t statsBox_j1();

    /// Get the minimum calibrated value in the stats box.
    /**
     * \returns m_statsBox_min
     */
    float statsBox_min();

    /// Get the maximum calibrated value in the stats box.
    /**
     * \returns m_statsBox_max
     */
    float statsBox_max();

    /// Get the mean calibrated value in the stats box.
    /**
     * \returns m_statsBox_mean
     */
    float statsBox_mean();

    /// Get the median calibrated value in the stats box.
    /**
     * \returns m_statsBox_median
     */
    float statsBox_median();

    /// Calculate stats-box values while not holding \ref m_calMutex.
    void mtxUL_calcStatsBox();

    /// Request updated stats-box values.
    /** Local mode resolves this request immediately and emits
     * rtimvBaseObject::statsBoxUpdated.
     */
    void requestStatsBoxValues();

    /// Calculate stats-box values with \ref m_calMutex in shared lock.
    void mtxL_calcStatsBox( const sharedLockT &lock /**<[in] a shared mutex lock which is locked on m_calMutex*/ );

    ///@}

    /** @name Color Stretch - Data
     *
     * @{
     */
  protected:
    rtimv::stretch m_stretch{ rtimv::stretch::linear };

    ///@}

    /** @name Color Stretch
     *
     * @{
     */
  public:
    void stretch( rtimv::stretch );

    rtimv::stretch stretch();

    typedef int ( *pixelIndexF )( float );

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
    float m_minScaleData{ 0 }; ///< The minimum data value used for scaling

    float m_maxScaleData{ 0 }; ///< The maximum data valuse used for scaling

    bool m_autoScale{ false };

  public:
    void minScaleData( float md );

    float minScaleData();

    void maxScaleData( float md );

    float maxScaleData();

    void bias( float b );

    float bias();

    void bias_rel( float b );

    float bias_rel();

    void contrast( float c );

    float contrast();

    void contrast_rel( float cr );

    float contrast_rel();

    /// Set the auto scale flag
    /**
     * The cal mutex must be unlocked before calling
     */
    void mtxUL_autoScale( bool as /**< [in] the new value of the auto scale flag */ );

    /// Get the auto scale flag value
    /**
     * \returns the current value of m_autoScale
     */
    bool autoScale();

    /// Restretch the image based on the current colormode.
    void mtxUL_reStretch();

    ///@}

    /** @name Image Filtering - Data
     *
     * Controls for optional high-pass/low-pass image filtering.
     * @{
     */
  protected:
    rtimv::hpFilter m_hpFilter{ rtimv::hpFilter::gaussian }; ///< Selected high-pass filter type.

    float m_hpfFW{ 10 }; ///< Full width for the high-pass filter in pixels.

    bool m_applyHPFilter{ false }; ///< Whether the high-pass filter is currently enabled.

    rtimv::lpFilter m_lpFilter{ rtimv::lpFilter::none }; ///< Selected low-pass filter type.

    float m_lpfFW{ 3 }; ///< Full width for the low-pass filter in pixels.

    bool m_applyLPFilter{ false }; ///< Whether the low-pass filter is currently enabled.

    ///@}

    /** @name Image Filtering
     *
     * Public access to image filtering configuration.
     * @{
     */
  public:
    /// Set the high-pass filter type.
    void hpFilter( rtimv::hpFilter filter /**< [in] selected high-pass filter type */ );

    /// Get the high-pass filter type.
    rtimv::hpFilter hpFilter();

    /// Set the high-pass filter full width.
    void hpfFW( float fw /**< [in] high-pass filter width in pixels */ );

    /// Get the high-pass filter full width.
    float hpfFW();

    /// Set whether high-pass filtering is applied.
    void applyHPFilter( bool apply /**< [in] true enables high-pass filtering */ );

    /// Get whether high-pass filtering is enabled.
    bool applyHPFilter();

    /// Set the low-pass filter type.
    void lpFilter( rtimv::lpFilter filter /**< [in] selected low-pass filter type */ );

    /// Get the low-pass filter type.
    rtimv::lpFilter lpFilter();

    /// Set the low-pass filter full width.
    void lpfFW( float fw /**< [in] low-pass filter width in pixels */ );

    /// Get the low-pass filter full width.
    float lpfFW();

    /// Set whether low-pass filtering is applied.
    void applyLPFilter( bool apply /**< [in] true enables low-pass filtering */ );

    /// Get whether low-pass filtering is enabled.
    bool applyLPFilter();

    ///@}

    /** @name Image Filtering - Working Memory
     *
     * @{
     */
    /// Scratch image used for intermediate smoothing during filtering.
    mx::improc::eigenImage<float> m_filterWork;

    /// Buffer holding the current high-pass filtered image.
    mx::improc::eigenImage<float> m_hpFiltered;

    /// Buffer holding the current low-pass filtered image.
    mx::improc::eigenImage<float> m_lpFiltered;
    /// @}

    //****** The display *************
  protected:
    QImage *m_qim{ nullptr }; ///< A QT image, used to store the color-map encoded data

    std::vector<uint8_t> m_lightness; ///< The perceived lightness values of the colormap RGB values

  private:
    /// An internal mutex to track if image data is being changed
    /**
     * This is used to avoid unecessary/redundant recolors
     */
    std::mutex m_changingImData;

  public:
    /// Get a reference the lightness data
    const std::vector<uint8_t> &lightness();

    /// Get the lightness for a specific index
    uint8_t lightness( size_t n /**< [in] the color table index */ );

    /// Get the lightness for a pixel
    uint8_t lightness( int x, /**< [in] the x location of the pixel */
                       int y  /**< [in] the y location of the pixel */
    );

    /// Updates filtered data, the QImage, and basic statistics.
    /** When \p newdata is false, raw calibrated pixels in \ref m_calDataRaw are reused.
     */
    void mtxUL_changeImdata( bool newdata = true /**< [in] true to repopulate \ref m_calDataRaw */ );

    /// Apply configured high-pass/low-pass filters and update \ref m_calData.
    /** Called with \ref m_calMutex in shared-lock context from \ref mtxUL_changeImdata.
     */
    void mtxL_applyFilter();

  private:
    /// Color the image based on the current colormap configuration and stretch
    /** This uses the specified color scale (linear, log, etc), and the
     * specified stretch.  If all pixels are equal they are all set to 0.
     * Sets NaN pixels to the \ref m_nanColor.
     *
     * Also sets any saturated pixels to \ref m_satColor.
     *
     */
    void mtxL_recolorImpl();

  public:
    /// Perform a recolor when \ref m_calMutex is not yet locked
    /**
     * This takes a shared lock on \ref m_calMutex then calls \ref mtxL_recolor()
     *
     */
    void mtxUL_recolor();

    /// Perform a recolor with \ref m_calMutex i in shared lock
    /**
     * Calls \ref mtxL_recolorImpl()
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
    float m_minImageData; ///< The minimum value of the calibrated image data
    float m_maxImageData; ///< The maximum value of the calibrated image data

  public:
    uint32_t saturated()
    {
        return m_saturated;
    }

    /// Get the current minimum calibrated value
    /**
     * \returns the current value of m_minImageData
     */
    float minImageData();

    /// Get the current maximum calibrated value
    /**
     * \returns the current value of m_maxImageData
     */
    float maxImageData();

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

    /*** Real Time Controls ***/
  protected:
    bool m_realTimeStopped{ false }; ///< Set when user temporarily freezes real-time data viewing.

  public:
    /// Get whether real-time is being used
    /**
     * \returns the current value of m_realTimeStopped.
     */
    bool realTimeStopped();

    /// Set whether to temporarily freeze real-time data viewing
    void realTimeStopped( bool rts /**< [in] the new value for m_realTimeStopped */ );

    virtual void updateFPS(); ///< Called whenever the displayed image updates its FPS.
    virtual void updateAge(); ///< Called whenever the displayed image updates its Age.
    virtual void updateNC();  ///< Update the display while not connected.
};

#endif // rtimv_rtimvBase_hpp
