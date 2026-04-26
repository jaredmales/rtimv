/** \file rtimvServerThread.hpp
 * \brief Declarations for the rtimvServerThread class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimvServerThread_hpp
#define rtimvServerThread_hpp

#include "rtimvBase.hpp"

#include <grpcpp/grpcpp.h>

#include <QThread>
#include <QByteArray>

#include <mx/app/application.hpp>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

#include "rtimv.grpc.pb.h"

using namespace mx::app;

#define RTIMV_DEBUG_BREADCRUMB

class rtimvServerThread : public QThread, public rtimvBase
{
    Q_OBJECT

  protected:
    std::string m_uri; ///< The URI for this thread as a server client

    std::shared_ptr<std::vector<std::string>> m_argv; /**< The command line argv.  This is set in the constructor, and
                                                      destroyed after configure is called.*/

    /// Default JPEG quality to use when no per-image startup quality is configured.
    int m_defaultQuality{ 50 };

    /// Configured startup JPEG quality for this image stream.
    int m_startupQuality{ 50 };

    /// True when startup JPEG quality was explicitly configured.
    bool m_startupQualitySet{ false };

    std::atomic<int> m_quality{ 50 }; ///< The JPEG quality factor (0-100).  Default is 50.

    std::atomic<double> m_lastRequest{ 0 }; ///< The time of the last request for an image

    std::atomic<bool> m_asleep{ false };

    std::atomic<int> m_configured{ 0 }; ///< 0 is unconfigured, 1 is configured, -1 is configuration error

    std::atomic<uint32_t> m_activeRpc{ 0 }; ///< Number of active RPC handlers currently using this thread.

    std::string m_calledName{ "rtimvServer" }; ///< Program called-name used in standardized log prefixes.

    bool m_includeAppName{ true }; ///< True to include called-name in log prefixes.

    /// True when a newer response than the last delivered reply is ready to send.
    std::atomic<bool> m_responseDirty{ false };

    /// Monotonic serial assigned to each newly renderable response state.
    std::atomic<uint64_t> m_responseSerial{ 0 };

    /// True while a queued pending-image service pass is already scheduled.
    std::atomic<bool> m_servicePendingScheduled{ false };

    /// Custom unary reactor used for queued ImagePlease replies.
    class pendingImageReactor;

    /// One queued ImagePlease request waiting for the next deliverable response.
    struct pendingImageRequest
    {
        pendingImageReactor *m_reactor{ nullptr }; ///< Custom reactor finished when this pending request is served.

        remote_rtimv::Image *m_reply{ nullptr }; ///< Reply payload populated when the request is fulfilled.

        double m_enqueueTime{ 0 }; ///< Monotonic enqueue time in seconds for timeout accounting.

        std::atomic<bool> m_cancelled{ false }; ///< True once gRPC reports client-side cancellation for this request.

        std::atomic<bool> m_done{ false }; ///< True once gRPC has fully completed this request.

        std::atomic<bool> m_finished{ false }; ///< True once Finish has been issued for this request.
    };

    /// Mutex guarding the pending ImagePlease queue.
    std::mutex m_pendingImageMutex;

    /// FIFO queue of pending ImagePlease requests representing client-side credits.
    std::deque<std::shared_ptr<pendingImageRequest>> m_pendingImageRequests;

    /// Mark the current rendered response dirty and advance the response serial.
    void markResponseDirty();

    /// Populate reply metadata from the current server-thread state.
    void populateImageReply( remote_rtimv::Image *reply /**< [in] reply payload to populate */ );

    /// Schedule queued servicing of pending image requests on the Qt side.
    void schedulePendingImageService();

  public:
    rtimvServerThread( const std::string &uri,                         /**< [in] client uri */
                       std::shared_ptr<std::vector<std::string>> argv, /**< [in] The argv vector. */
                       int defaultQuality,            /**< [in] default JPEG quality when not configured per image */
                       const std::string &calledName, /**< [in] Program called-name for log prefixes. */
                       bool includeAppName,           /**< [in] True to include called-name in log prefixes. */
                       QObject *parent = nullptr      /**< [in] [opt] the parent of this thread */
    );

    ~rtimvServerThread();

    /// Register server-thread-specific config options, including per-image JPEG quality.
    virtual void setupConfig();

    /// Load server-thread-specific config values after the base image configuration is parsed.
    virtual void loadConfig();

    void configure();

    int configured();

  signals:
    void rendered();

  public:
    /// Called on initial connection to the image stream, sets matching aspect ratio.
    virtual void onConnect();

    virtual void mtxL_postSetImsize( const uniqueLockT &lock );

    virtual void post_zoomLevel();

  public:
    void mtxUL_recolor();

    virtual void mtxL_postRecolor( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ );

    virtual void mtxL_postRecolor( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    virtual void mtxL_postChangeImdata( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    virtual void mtxL_postColormode( rtimv::colormode m, const sharedLockT &lock );

    void mtxuL_render( std::string *image /**< [out] rendered JPEG payload */ );

    /// Apply a new minimum image timeout on the Qt event thread.
    void setImageTimeout( int to /**< [in] the new minimum image timeout in ms */ );

    int quality();

    void quality( int q );

    double lastRequest();

    void lastRequest( double lr );

    double sinceLastRequest();

    bool asleep();

    /// Increment active RPC counter.
    void rpcBegin();

    /// Decrement active RPC counter.
    void rpcEnd();

    /// Get current active RPC count.
    uint32_t rpcActive();

    /// Queue one pending ImagePlease request for the next deliverable response.
    void enqueueImageRequest( std::shared_ptr<pendingImageRequest> request /**< [in] pending request to enqueue */
    );

    /// Create and enqueue a custom unary reactor for one ImagePlease request.
    grpc::ServerUnaryReactor *
    newImagePleaseReactor( remote_rtimv::Image *reply /**< [in] reply payload owned by gRPC */ );

    /// Drop cancelled pending requests and expire old ones.
    void prunePendingImageRequests( double timeoutSeconds /**< [in] timeout applied to queued requests in seconds */ );

    /// Get the number of queued pending ImagePlease requests.
    size_t pendingImageRequests();

  signals:
    void servicePending();

    void gotosleep();

    void awaken();

  public:
    void emit_gotosleep();

    void emit_awaken();

  public slots:
    /// Attempt to serve one pending ImagePlease request with the newest available response.
    void servicePendingImageRequests();

    void sleep();

    void wakeup();
};

#endif // rtimvServerThread_hpp
