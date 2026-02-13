/** \file rtimvClientBase.hpp
 * \brief Declarations for the rtimvClientBase base class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvClientBase_hpp
#define rtimv_rtimvClientBase_hpp

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

#include <QImage>

#include <mx/app/application.hpp>

// #define RTIMV_BASE rtimvBase

#include "rtimvBaseObject.hpp"
#include "rtimvColor.hpp"

#include <grpcpp/grpcpp.h>

#include "rtimv.grpc.pb.h"

/*using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using remote_rtimv::Config;
using remote_rtimv::ConfigResult;
using remote_rtimv::Image;
using remote_rtimv::ImageRequest;
using remote_rtimv::ImageStatus;

using remote_rtimv::Coord;
using remote_rtimv::Pixel;

using remote_rtimv::rtimv;*/

/// The base class for rtimvClient functions

class rtimvClientBase : public mx::app::application
{

  public:
    typedef std::unique_lock<std::shared_mutex> uniqueLockT;

    typedef std::shared_lock<std::shared_mutex> sharedLockT;

  public:
    /** @name Construction
     *
     * @{
     */

    /// Basic c'tor.
    rtimvClientBase();

    ~rtimvClientBase();

    // static rtimvClientBase * globalBase;

    /// @}

  protected:
    /** @name Configuration
     *
     *  The mx::app::application interface for command line and config files.
     * @{
     */

    remote_rtimv::Config *m_configReq{ nullptr };

    std::string m_server{ "localhost" };
    int m_port{ 7000 };

    virtual void setupConfig();

    virtual void loadStandardConfig();

    virtual void loadConfig();

    ///@}

    rtimvBaseObject *m_foundation{ nullptr };

    bool m_imageWaiting{ false };

    /** @name Connection Data
     *
     * @{
     */

  protected:
    /// Flag used to indicate that the client is connected to the server.  Setting to false triggers an attempt to
    /// reconnect.
    bool m_connected{ false };

    /// Counter to track connection attemps.  Used to prevent threads from re-requesting re-connections for the same
    /// event.
    uint64_t m_connections{ 0 };

    /// Flag to prevent repeating connection failure reports.
    bool m_connectionFailReported{ false };

    std::shared_mutex m_connectedMutex;

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

  public slots:

    void reconnect();

  protected:
    /// Context for the ImagePlease rpc.
    /** This has to stay alive until the rpc finishes and can not be reused
     *
     */
    grpc::ClientContext *m_ImagePleaseContext{ nullptr };

  public:
    /// Configure the server
    /**
     */
    void Configure();

  protected:
    /// Mutex guarding asynchronous image request state.
    std::mutex m_imageRequestMutex;

    /// Condition variable used to signal completion of pending image requests.
    std::condition_variable m_imageRequestCv;

    /// Flag indicating client teardown is in progress.
    bool m_shuttingDown{ false };

    /// True while an ImagePlease RPC is outstanding.
    bool m_imageRequestPending{ false };

    /// Request payload for the ImagePlease RPC.
    remote_rtimv::ImageRequest m_grpcImageRequest;

    /// Last ImagePlease response payload from the server.
    remote_rtimv::Image m_grpcImage;

  public:
    /// Request an image from the server
    void ImagePlease();

    /// Process a received image
    void ImageReceived();

  protected:
    /// Handle an ImagePlease response from the server
    void ImagePlease_callback( grpc::Status status );

    /// Function called on connection
    /**
     * This function must set m_connected to true if successful.
     */
    virtual void onConnect()
    {
        m_connected = true;
    }

    ///@}

    std::shared_mutex m_calMutex;

    /** @name Image Status - Data
     * @{
     */
  protected:
    float m_fpsEst;

    double m_imageTime{ 0 };

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
    virtual void mtxL_postSetImsize( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ ) // = 0;
    {
    }

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

    float m_desiredCubeFPS{ 20 }; ///< The cube frame rate

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
     * cause the display to update at 20 f.p.s. (the default setting).
     *
     * For the client this is also limited by network transfer
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

  public:
    void subtractDark( bool sd );

    bool subtractDark();

    void applyMask( bool amsk );

    bool applyMask();

    void applySatMask( bool asmsk );

    bool applySatMask();

    /// Get the value of a calibrated pixel
    /**
     * \returns the value of the (x,y) pixel in \ref m_calData
     */
    float calPixel( uint32_t x, /**< [in] the x coordinate of the pixel */
                    uint32_t y /**< [in] the y coordinate of the pixel */ );

    ///@}

    /** @name Color Bar
     *
     * @{
     */

    rtimv::colorbar m_colorbar{ rtimv::colorbar::bone };

  private:
    /// Actual implementation of loading the color bar
    void mtxL_load_colorbarImpl( rtimv::colorbar cb, /**< [in] the new color bar */
                                 bool update         /**< [in] whether or not to update the image*/
    );

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

    void mtxUL_reStretch();

    ///@}

    /** @name Image Filtering
     *
     * @{
     */

    // float *m_lowPassFiltered{ nullptr };

    // bool m_applyLPFilter;

    // int m_lpFilterType;

    ///@} -- filtering

    //****** The display *************
  protected:
    QImage *m_qim{ nullptr }; ///< A QT image, used to store the color-map encoded data

  public:
    /// Get the lightness for a pixel
    uint8_t lightness( int x, /**< [in] the x location of the pixel */
                       int y  /**< [in] the y location of the pixel */
    );

    /// Perform a recolor when \ref m_calMutex is not yet locked
    /**
     * This takes a shared lock on \ref m_calMutex then calls \ref mtxL_recolor()
     *
     */
    void mtxUL_recolor();

    /// Perform a recolor when \ref m_calMutex is in shared lock
    /**
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
     * will normally want to implement a single function.
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
    virtual void mtxL_postRecolor( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ ) = 0;

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
    // float m_satLevel{ 1e30 };
    uint32_t m_saturated{ 0 };

    /* Image Stats */
  protected:
    float m_minImageData{ 0 }; ///< The minimum value of the calibrated image data
    float m_maxImageData{ 0 }; ///< The maximum value of the calibrated image data

  public:
    uint32_t saturated();

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
    /// Get the current zoom level
    float zoomLevel();

    /// Get the minimum zoom level
    float zoomLevelMin();

    /// Get the maximum zoom level
    float zoomLevelMax();

    /// Set the zoom level
    void zoomLevel( float zl /**< the new zoom level */ );

    /// Carry out any needed display actions after setting zoom level
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

  private:
    std::unique_ptr<remote_rtimv::rtimv::Stub> stub_;
};

#endif // rtimv_rtimvClientBase_hpp
