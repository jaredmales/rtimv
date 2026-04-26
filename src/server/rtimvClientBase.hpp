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
#include <chrono>
#include <deque>
#include <memory>
#include <string_view>
#include <unordered_map>

#include <QImage>

#include <mx/app/application.hpp>

// #define RTIMV_BASE rtimvBase

#include "rtimvBaseObject.hpp"
#include "rtimvColor.hpp"
#include "rtimvFilters.hpp"

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

    bool m_logAppName{ true }; ///< True to include called-name in log prefixes.

    std::string m_calledName{ "rtimvClient" }; ///< Program called-name used in standardized log prefixes.

    std::string m_clientId; ///< Client id returned by server Configure, if provided.

    virtual void setupConfig();

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
    /// Get the main image name/key for log context.
    std::string logImage0() const;

    /// Format a standardized client log message.
    std::string
    formatBaseLogMessage( std::string_view message /**< [in] text to append after standardized prefix */ ) const;

    /// Refresh configured image names from the server.
    void updateImageNamesFromServer();

    /// State for one in-flight ImagePlease RPC.
    struct imageRequestState
    {
        std::unique_ptr<grpc::ClientContext> m_context; ///< RPC context kept alive until the callback completes.

        remote_rtimv::ImageRequest m_request; ///< Request payload for this ImagePlease call.

        remote_rtimv::Image m_reply; ///< Response payload returned by the server for this ImagePlease call.

        uint64_t m_connectionGeneration{ 0 }; ///< Connection generation active when this request was dispatched.
    };

    /// Context for the GetPixel rpc.
    grpc::ClientContext *m_GetPixelContext{ nullptr };

    /// Context for the ColorBox rpc.
    grpc::ClientContext *m_ColorBoxContext{ nullptr };

    /// Context for the StatsBox rpc.
    grpc::ClientContext *m_StatsBoxContext{ nullptr };

    /// Context for the SetColorstretch rpc.
    grpc::ClientContext *m_SetColorstretchContext{ nullptr };

    /// Context for the SetMinScale rpc.
    grpc::ClientContext *m_SetMinScaleContext{ nullptr };

    /// Context for the SetMaxScale rpc.
    grpc::ClientContext *m_SetMaxScaleContext{ nullptr };

    /// Context for the Ping rpc.
    grpc::ClientContext *m_PingContext{ nullptr };

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

    /// Desired steady-state number of in-flight ImagePlease RPCs after the first valid image arrives.
    size_t m_targetImageWindow{ 10 };

    /// True once at least one valid image has been received and the full image-credit window can be used.
    bool m_imagePipelinePrimed{ false };

    /// Next unique identifier assigned to an ImagePlease request.
    uint64_t m_nextImageRequestId{ 1 };

    /// In-flight ImagePlease RPCs keyed by their local request identifier.
    std::unordered_map<uint64_t, std::shared_ptr<imageRequestState>> m_inflightImageRequests;

    /// Completed ImagePlease replies waiting to be applied on the Qt side.
    std::deque<remote_rtimv::Image> m_completedImageReplies;

    /// Monotonic serial of the newest Image reply already applied to the client state.
    uint64_t m_lastAppliedResponseSerial{ 0 };

    /// Backoff delay for ImagePlease retries when no new image data is available, ms.
    int m_imageRetryBackoffMs{ 500 };

    /// Mutex guarding asynchronous unary RPC state for Ping/GetPixel/ColorBox/StatsBox.
    std::mutex m_asyncRpcMutex;

    /// Condition variable used to signal completion of Ping/GetPixel/ColorBox/StatsBox requests.
    std::condition_variable m_asyncRpcCv;

    /// Request payload for Ping.
    remote_rtimv::PingRequest m_pingRequest;

    /// Response payload for Ping.
    remote_rtimv::PingResponse m_pingReply;

    /// True while a Ping request is outstanding.
    bool m_pingPending{ false };

    /// Monotonic send time for the outstanding Ping request.
    std::chrono::steady_clock::time_point m_pingStartTime;

    /// True while a GetPixel request is outstanding.
    bool m_getPixelPending{ false };

    /// True when a newer GetPixel request should be sent after current completion.
    bool m_getPixelQueued{ false };

    /// Current desired x coordinate for GetPixel.
    uint32_t m_getPixelX{ 0 };

    /// Current desired y coordinate for GetPixel.
    uint32_t m_getPixelY{ 0 };

    /// In-flight x coordinate for the outstanding GetPixel request.
    uint32_t m_getPixelInflightX{ 0 };

    /// In-flight y coordinate for the outstanding GetPixel request.
    uint32_t m_getPixelInflightY{ 0 };

    /// Request payload for GetPixel.
    remote_rtimv::Coord m_getPixelRequest;

    /// Response payload for GetPixel.
    remote_rtimv::Pixel m_getPixelReply;

    /// True while a ColorBox request is outstanding.
    bool m_colorBoxPending{ false };

    /// True when a newer ColorBox request should be sent after current completion.
    bool m_colorBoxQueued{ false };

    /// Request payload for ColorBox.
    remote_rtimv::Box m_colorBoxRequest;

    /// Response payload for ColorBox.
    remote_rtimv::MinvalMaxval m_colorBoxReply;

    /// In-flight upper-left x coordinate for ColorBox.
    int64_t m_colorBoxInflight_i0{ 0 };

    /// In-flight lower-right x coordinate for ColorBox.
    int64_t m_colorBoxInflight_i1{ 0 };

    /// In-flight upper-left y coordinate for ColorBox.
    int64_t m_colorBoxInflight_j0{ 0 };

    /// In-flight lower-right y coordinate for ColorBox.
    int64_t m_colorBoxInflight_j1{ 0 };

    /// True while a StatsBox request is outstanding.
    bool m_statsBoxPending{ false };

    /// True when a newer StatsBox request should be sent after current completion.
    bool m_statsBoxQueued{ false };

    /// Request payload for StatsBox.
    remote_rtimv::Box m_statsBoxRequest;

    /// Response payload for StatsBox.
    remote_rtimv::StatsValues m_statsBoxReply;

    /// In-flight upper-left x coordinate for StatsBox.
    int64_t m_statsBoxInflight_i0{ 0 };

    /// In-flight lower-right x coordinate for StatsBox.
    int64_t m_statsBoxInflight_i1{ 0 };

    /// In-flight upper-left y coordinate for StatsBox.
    int64_t m_statsBoxInflight_j0{ 0 };

    /// In-flight lower-right y coordinate for StatsBox.
    int64_t m_statsBoxInflight_j1{ 0 };

    /// Request payload for SetColorstretch.
    remote_rtimv::ColorstretchRequest m_setColorstretchRequest;

    /// Response payload for SetColorstretch.
    remote_rtimv::ColorstretchResponse m_setColorstretchReply;

    /// True while a SetColorstretch request is outstanding.
    bool m_setColorstretchPending{ false };

    /// True when a newer SetColorstretch request should be sent after current completion.
    bool m_setColorstretchQueued{ false };

    /// Current desired stretch to apply.
    rtimv::stretch m_setColorstretchDesired{ rtimv::stretch::linear };

    /// Request payload for SetMinScale.
    remote_rtimv::ScaleRequest m_setMinScaleRequest;

    /// Response payload for SetMinScale.
    remote_rtimv::ScaleResponse m_setMinScaleReply;

    /// True while a SetMinScale request is outstanding.
    bool m_setMinScalePending{ false };

    /// True when a newer SetMinScale request should be sent after current completion.
    bool m_setMinScaleQueued{ false };

    /// Current desired minimum scale value.
    float m_setMinScaleDesired{ 0 };

    /// In-flight minimum scale value.
    float m_setMinScaleInflight{ 0 };

    /// Last observed minimum scale at SetMinScale send time.
    float m_setMinScalePrevAtSend{ 0 };

    /// True while waiting for ImagePlease to reflect the in-flight minimum scale update.
    bool m_setMinScaleAwaitImage{ false };

    /// Request payload for SetMaxScale.
    remote_rtimv::ScaleRequest m_setMaxScaleRequest;

    /// Response payload for SetMaxScale.
    remote_rtimv::ScaleResponse m_setMaxScaleReply;

    /// True while a SetMaxScale request is outstanding.
    bool m_setMaxScalePending{ false };

    /// True when a newer SetMaxScale request should be sent after current completion.
    bool m_setMaxScaleQueued{ false };

    /// Current desired maximum scale value.
    float m_setMaxScaleDesired{ 0 };

    /// In-flight maximum scale value.
    float m_setMaxScaleInflight{ 0 };

    /// Last observed maximum scale at SetMaxScale send time.
    float m_setMaxScalePrevAtSend{ 0 };

    /// True while waiting for ImagePlease to reflect the in-flight maximum scale update.
    bool m_setMaxScaleAwaitImage{ false };

    /// Contexts for fire-and-forget empty-response unary RPCs.
    std::vector<grpc::ClientContext *> m_emptyRpcContexts;

    /// Number of in-flight fire-and-forget empty-response unary RPCs.
    size_t m_emptyRpcPending{ 0 };

    /// Mutex guarding the count of callback threads still actively executing client code.
    std::mutex m_callbackActivityMutex;

    /// Condition variable signaled when an active gRPC callback finishes using this client object.
    std::condition_variable m_callbackActivityCv;

    /// Number of gRPC callback functions currently executing client-side follow-up logic.
    size_t m_activeGrpcCallbacks{ 0 };

  public:
    /// Request an image from the server
    void ImagePlease();

  protected:
    /// Record entry into a gRPC callback so shutdown can wait for client-side callback work to finish.
    void beginGrpcCallbackActivity();

    /// Record exit from a gRPC callback and wake shutdown waiters when the active count reaches zero.
    void endGrpcCallbackActivity();

    /// Launch the asynchronous Ping RPC used for transport RTT measurement.
    void dispatchPingAsync();

    /// Launch one asynchronous ImagePlease RPC state.
    virtual void
    dispatchImagePleaseAsync( uint64_t requestId, /**< [in] local identifier for the in-flight request */
                              std::shared_ptr<imageRequestState> state /**< [in] owned request/reply state */
    );

  public:
    /// Request an updated calibrated pixel value.
    void requestPixelValue( uint32_t x, /**< [in] x coordinate of the pixel */
                            uint32_t y /**< [in] y coordinate of the pixel */ );

    /// Request updated color-box min/max values.
    void requestColorBoxValues();

    /// Request updated stats-box values.
    void requestStatsBoxValues();

    /// Process a received image
    void ImageReceived();

  protected:
    /// Handle a Ping response from the server.
    void Ping_callback( grpc::Status status );

    /// Handle an ImagePlease response from the server.
    void ImagePlease_callback( uint64_t requestId, /**< [in] local identifier for the completed request */
                               std::shared_ptr<imageRequestState> state, /**< [in] owned request/reply state */
                               grpc::Status status                       /**< [in] completion status */
    );

    /// Handle a GetPixel response from the server.
    void GetPixel_callback( grpc::Status status );

    /// Handle a ColorBox response from the server.
    void ColorBox_callback( grpc::Status status );

    /// Handle a StatsBox response from the server.
    void StatsBox_callback( grpc::Status status );

    /// Handle a SetColorstretch response from the server.
    void SetColorstretch_callback( grpc::Status status );

    /// Handle a SetMinScale response from the server.
    void SetMinScale_callback( grpc::Status status );

    /// Handle a SetMaxScale response from the server.
    void SetMaxScale_callback( grpc::Status status );

    /// Handle completion of a fire-and-forget empty-response unary RPC.
    void EmptyRpc_callback( grpc::ClientContext *context, /**< [in] context for the completed RPC */
                            grpc::Status status /**< [in] completion status */ );

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
    /// Configured image names/keys by image index, used before receiving first image.
    /** By index:
     * - 0 is the main image.
     * - 1 is the dark image which is (optionally) subtracted from the main image.
     * - 2 is the mask image which is (optionally) multiplied by the dark-subtracted image.  Normally a 1/0 image.
     * - 3 is the saturation mask which (optionally) denotes which pixels to turn the saturation color.
     */
    std::vector<std::string> m_imageNames;

    float m_fpsEst;

    int m_rollingStatsFrames{ 10 }; ///< Number of frames used for rolling transport-stat averages.

    double m_lastCompressionRatio{ 0 }; ///< Compression ratio of the most recently received image payload.

    double m_avgCompressionRatio{ 0 }; ///< Rolling average compression ratio over recent received frames.

    double m_avgFrameRate{ 0 }; ///< Rolling average frame rate most recently published for display.

    double m_lastRttMs{ 0 }; ///< Round-trip time of the most recently completed Ping RPC, in ms.

    double m_avgRttMs{ 0 }; ///< Rolling average Ping round-trip time, in ms.

    double m_fpsRollingSum{ 0 }; ///< Running sum for the rolling frame-rate average.

    double m_compressionRollingSum{ 0 }; ///< Running sum for the rolling compression-ratio average.

    double m_rttRollingSum{ 0 }; ///< Running sum for the rolling RTT average.

    std::deque<double> m_recentFrameIntervals; ///< Recent inter-arrival times used to form the rolling average.

    std::deque<double> m_recentCompressionRatios; ///< Recent compression-ratio samples used to form the average.

    std::deque<double> m_recentRtts; ///< Recent Ping RTT samples used to form the rolling average.

    std::chrono::steady_clock::time_point m_lastArrivalTime; ///< Monotonic arrival time of the previous received frame.

    std::chrono::steady_clock::time_point
        m_lastFrameRatePublishTime; ///< Monotonic time when m_avgFrameRate was last refreshed for display.

    double m_imageTime{ 0 };

    uint32_t m_imageNo{ 0 };

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

    int m_quality{ 50 }; ///< JPEG quality factor [0,100].

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
    /// Update rolling RTT statistics from a completed Ping RPC.
    void updateRollingRttStats( double rttMs /**< [in] round-trip time in milliseconds */ );

    void setCurrImageTimeout();

    /// Update rolling transport statistics from the latest received frame.
    void updateRollingTransportStats( uint32_t nx /**< [in] frame width in pixels */,
                                      uint32_t ny /**< [in] frame height in pixels */,
                                      size_t compressedBytes /**< [in] compressed JPEG payload size in bytes */,
                                      uint32_t sourceBytesPerPixel /**< [in] native source bytes per pixel */,
                                      bool validPayload /**< [in] true if a JPEG payload was received and decoded */
    );

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

    /// Set the JPEG quality.
    void quality( int q /**< [in] the new JPEG quality [0,100] */ );

    /// Get the JPEG quality
    int quality();

    /// Set the target number of in-flight ImagePlease RPCs.
    void imageRequestWindow( int window /**< [in] desired in-flight ImagePlease window */ );

    /// Get the target number of in-flight ImagePlease RPCs.
    int imageRequestWindow();

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

    /// Get the compression ratio of the most recently received frame.
    double lastCompressionRatio();

    /// Get the rolling average compression ratio.
    double avgCompressionRatio();

    /// Get the rolling average frame rate.
    double avgFrameRate();

    /// Get the RTT of the most recently completed Ping RPC.
    double lastRttMs();

    /// Get the rolling average Ping RTT.
    double avgRttMs();

    /// @}

    /** @name Image Update - Slots
     *
     *  These aren't actually slots, but are the callbacks from the BaseObject's slots (m_foundation)
     *
     * @{
     */
  public:
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
     * Settings to control which calibrations are applied.
     *
     * In the gRPC client, these values are synchronized from the server via
     * the Image message and are also writable through corresponding RPCs.
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

    /** @name Stats Box - Data
     *
     * @{
     */
  private:
    /// The stats box upper left corner x coordinate.
    int64_t m_statsBox_i0{ 0 };

    /// The stats box lower right corner x coordinate.
    int64_t m_statsBox_i1{ 0 };

    /// The stats box upper left corner y coordinate.
    int64_t m_statsBox_j0{ 0 };

    /// The stats box lower right corner y coordinate.
    int64_t m_statsBox_j1{ 0 };

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

    /// Request stats-box calculations from the server.
    void mtxUL_calcStatsBox();

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
    float m_hpfFW{ 10 };                                     ///< Full width for the high-pass filter in pixels.
    bool m_applyHPFilter{ false };                           ///< Whether the high-pass filter is currently enabled.

    rtimv::lpFilter m_lpFilter{ rtimv::lpFilter::none }; ///< Selected low-pass filter type.
    float m_lpfFW{ 3 };                                  ///< Full width for the low-pass filter in pixels.
    bool m_applyLPFilter{ false };                       ///< Whether the low-pass filter is currently enabled.

    /// Filtering working buffers are not stored on the client.
    /** The client receives post-filter state through the Image message; filtering work
     * is performed on the server side.
     */

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
