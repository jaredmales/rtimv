
/** \file rtimvServer.hpp
 * \brief Declarations for the rtimvServer class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimvServer_hpp
#define rtimvServer_hpp

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <format>
#include <mutex>
#include <shared_mutex>

#include "rtimv.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::Status;

#include <mx/app/application.hpp>

#include "rtimvServerThread.hpp"
#include "rtimvColorGRPC.hpp"
#include "rtimvFilterGRPC.hpp"

#define RTIMV_DEBUG_BREADCRUMB

class rtimvServer : public QObject, public mx::app::application, public remote_rtimv::rtimv::CallbackService
{
    Q_OBJECT

  public:
    /// Type of mutex lock used for insertion and deletion
    typedef std::unique_lock<std::shared_mutex> uniqueLockT;

    /// Type of mutex lock used for read and write
    typedef std::shared_lock<std::shared_mutex> sharedLockT;

  protected:
    std::string m_serverAddress{ "0.0.0.0" }; ///< Host/interface address the grpc server listens on.

    int m_port{ 7000 }; ///< Port the grpc server listens on.

    float m_waitTimeout = 1; ///< Time to wait for a new image to be ready before timing out, in s.  Default is 1 s.

    int m_waitSleep = 100; ///< Time to sleep while waiting for a new image, in ms.  Default is 100 ms.

    float m_clientSleep{ 10 }; /**< Time in seconds after which a thread with no requests will be put
                                    to sleep.  Default is 10 s.*/

    float m_clientDisconnect{ 120 }; /**< Time in seconds after which a thread with no requests will be disconnected.
                                          Default is 120 s.*/

    int m_qualityDefault{ 50 }; ///< Default JPEG quality for new image threads when no per-image value is configured.

    bool m_logAppName{ true }; ///< True to include called-name in log prefixes.

    std::string m_calledName{ "rtimvServer" }; ///< Program called-name used in standardized log prefixes.

    /// Mutex for locking access to the client list
    /** Most uses require non-exclusive shared-locking for readings.
     *  Is exclusively locked for insertion and deletion.
     */
    std::shared_mutex m_clientMutex;

  public:
    /// Container for client configuration information during connections
    struct configSpec
    {
        std::string m_uri; ///< The URI of the client

        remote_rtimv::Config m_config;

        configSpec( const std::string uri, const remote_rtimv::Config *config ) : m_uri( uri )
        {
            if( config )
            {
                m_config = *config;
            }
        }
    };

    rtimvServer( int argc, char **argv, QObject *Parent = nullptr );

    ~rtimvServer();

    virtual void setupConfig();

    virtual void loadConfig();

    void startServer();

  protected:
    QThread *m_serverThread{ nullptr };

    typedef std::unordered_map<std::string, rtimvServerThread *>::value_type clientT;
    std::unordered_map<std::string, rtimvServerThread *> m_clients;

    ServerUnaryReactor *Configure( CallbackServerContext *context,
                                   const remote_rtimv::Config *request,
                                   remote_rtimv::ConfigResult *reply ) override;

    ServerUnaryReactor *SetColorbar( CallbackServerContext *context,
                                     const remote_rtimv::ColorbarRequest *request,
                                     remote_rtimv::ColorbarResponse *reply ) override;

    ServerUnaryReactor *SetColormode( CallbackServerContext *context,
                                      const remote_rtimv::ColormodeRequest *request,
                                      remote_rtimv::ColormodeResponse *reply ) override;

    ServerUnaryReactor *SetColorstretch( CallbackServerContext *context,
                                         const remote_rtimv::ColorstretchRequest *request,
                                         remote_rtimv::ColorstretchResponse *reply ) override;

    ServerUnaryReactor *SetMinScale( CallbackServerContext *context,
                                     const remote_rtimv::ScaleRequest *request,
                                     remote_rtimv::ScaleResponse *reply ) override;

    ServerUnaryReactor *SetMaxScale( CallbackServerContext *context,
                                     const remote_rtimv::ScaleRequest *request,
                                     remote_rtimv::ScaleResponse *reply ) override;

    ServerUnaryReactor *SetImageTimeout( CallbackServerContext *context,
                                         const remote_rtimv::ImageTimeoutRequest *request,
                                         remote_rtimv::ImageTimeoutResponse *reply ) override;

    ServerUnaryReactor *SetQuality( CallbackServerContext *context,
                                    const remote_rtimv::QualityRequest *request,
                                    remote_rtimv::QualityResponse *reply ) override;

    ServerUnaryReactor *Restretch( CallbackServerContext *context,
                                   const remote_rtimv::RestretchRequest *request,
                                   remote_rtimv::RestretchResponse *reply ) override;

    ServerUnaryReactor *SetAutoscale( CallbackServerContext *context,
                                      const remote_rtimv::AutoscaleRequest *request,
                                      remote_rtimv::AutoscaleResponse *reply ) override;

    ServerUnaryReactor *SetSubDark( CallbackServerContext *context,
                                    const remote_rtimv::SubDarkRequest *request,
                                    remote_rtimv::SubDarkResponse *reply ) override;

    ServerUnaryReactor *SetApplyMask( CallbackServerContext *context,
                                      const remote_rtimv::ApplyMaskRequest *request,
                                      remote_rtimv::ApplyMaskResponse *reply ) override;

    ServerUnaryReactor *SetApplySatMask( CallbackServerContext *context,
                                         const remote_rtimv::ApplySatMaskRequest *request,
                                         remote_rtimv::ApplySatMaskResponse *reply ) override;

    ServerUnaryReactor *SetHPFilter( CallbackServerContext *context,
                                     const remote_rtimv::HPFilterRequest *request,
                                     remote_rtimv::HPFilterResponse *reply ) override;

    ServerUnaryReactor *SetHPFW( CallbackServerContext *context,
                                 const remote_rtimv::FilterWidthRequest *request,
                                 remote_rtimv::FilterWidthResponse *reply ) override;

    ServerUnaryReactor *SetApplyHPFilter( CallbackServerContext *context,
                                          const remote_rtimv::ApplyFilterRequest *request,
                                          remote_rtimv::ApplyFilterResponse *reply ) override;

    ServerUnaryReactor *SetLPFilter( CallbackServerContext *context,
                                     const remote_rtimv::LPFilterRequest *request,
                                     remote_rtimv::LPFilterResponse *reply ) override;

    ServerUnaryReactor *SetLPFW( CallbackServerContext *context,
                                 const remote_rtimv::FilterWidthRequest *request,
                                 remote_rtimv::FilterWidthResponse *reply ) override;

    ServerUnaryReactor *SetApplyLPFilter( CallbackServerContext *context,
                                          const remote_rtimv::ApplyFilterRequest *request,
                                          remote_rtimv::ApplyFilterResponse *reply ) override;

    /// Respond to a client reachability ping without waking the image thread.
    ServerUnaryReactor *Ping( CallbackServerContext *context,
                              const remote_rtimv::PingRequest *request,
                              remote_rtimv::PingResponse *reply ) override;

    ServerUnaryReactor *ImagePlease( CallbackServerContext *context,
                                     const remote_rtimv::ImageRequest *request,
                                     remote_rtimv::Image *reply ) override;

    /// Set the cube playback direction on the server.
    ServerUnaryReactor *CubeDir( CallbackServerContext *context,
                                 const remote_rtimv::CubeDirRequest *request,
                                 remote_rtimv::CubeDirResponse *reply ) override;

    /// Set the cube frame on the server.
    ServerUnaryReactor *CubeFrame( CallbackServerContext *context,
                                   const remote_rtimv::CubeFrameRequest *request,
                                   remote_rtimv::CubeFrameResponse *reply ) override;

    ServerUnaryReactor *UpdateCube( CallbackServerContext *context,
                                    const remote_rtimv::UpdateCubeRequest *request,
                                    remote_rtimv::UpdateCubeResponse *reply ) override;

    ServerUnaryReactor *CubeFrameDelta( CallbackServerContext *context,
                                        const remote_rtimv::CubeFrameDeltaRequest *request,
                                        remote_rtimv::CubeFrameDeltaResponse *reply ) override;

    ServerUnaryReactor *
    GetPixel( CallbackServerContext *context, const remote_rtimv::Coord *request, remote_rtimv::Pixel *reply ) override;

    /// Get the image name/key for a requested image index.
    ServerUnaryReactor *GetImageName( CallbackServerContext *context,
                                      const remote_rtimv::ImageNameRequest *request,
                                      remote_rtimv::ImageNameResponse *reply ) override;

    /// Get info strings for a requested image index.
    ServerUnaryReactor *GetInfo( CallbackServerContext *context,
                                 const remote_rtimv::InfoRequest *request,
                                 remote_rtimv::InfoResponse *reply ) override;

    /// Get the image number for a requested image index.
    ServerUnaryReactor *GetImageNo( CallbackServerContext *context,
                                    const remote_rtimv::ImageNoRequest *request,
                                    remote_rtimv::ImageNoResponse *reply ) override;

    ServerUnaryReactor *ColorBox( CallbackServerContext *context,
                                  const remote_rtimv::Box *request,
                                  remote_rtimv::MinvalMaxval *reply ) override;

    ServerUnaryReactor *Recolor( CallbackServerContext *context,
                                 const remote_rtimv::RecolorRequest *request,
                                 remote_rtimv::RecolorResponse *reply ) override;

    ServerUnaryReactor *StatsBox( CallbackServerContext *context,
                                  const remote_rtimv::Box *request,
                                  remote_rtimv::StatsValues *reply ) override;

  signals:

    void gotConfigure( const configSpec * );

  protected slots:

    void doConfigure( const configSpec *cspec );
};

#endif // rtimvServer_hpp
