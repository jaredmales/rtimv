
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

using remote_rtimv::Config;
using remote_rtimv::ConfigResult;
using remote_rtimv::Image;
using remote_rtimv::ImageRequest;

using remote_rtimv::rtimv;

#include <mx/app/application.hpp>

#include "rtimvServerThread.hpp"

#define RTIMV_DEBUG_BREADCRUMB

class rtimvServer : public QObject, public mx::app::application, public rtimv::CallbackService
{
    Q_OBJECT

public:

    /// Type of mutex lock used for insertion and deletion
    typedef std::unique_lock<std::shared_mutex> uniqueLockT;

    /// Type of mutex lock used for read and write
    typedef std::shared_lock<std::shared_mutex> sharedLockT;

  protected:
    float m_waitTimeout = 1; ///< Time to wait for a new image to be ready before timing out, in s.  Default is 1 s.

    int m_waitSleep = 100;   ///< Time to sleep while waiting for a new image, in ms.  Default is 100 ms.

    float m_clientSleep{ 5 }; /**< Time in seconds after which a thread with no requests will be put
                                    to sleep.  Default is 10 s.*/

    float m_clientDisconnect{ 10 }; /**< Time in seconds after which a thread with no requests will be disconnected.
                                          Default is 120 s.*/

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

        std::string m_config; ///< The configuration file to read for this client

        std::string m_imageKey;

        std::string m_darkKey;

        std::string m_maskKey;

        std::string m_satMaskKey;

        float m_updateFPS {10};

        uint32_t m_updateTimeout { 100};

        float m_updateCubeFPS {10};

        bool m_autoscale {false};

        bool m_darksub {false};

        float m_satLevel {1e30};

        bool m_masksat {false};

        bool m_mzmqAlways {false};

        std::string m_mzmqServer;

        uint32_t m_port {0};

        configSpec( const std::string uri ) : m_uri( uri )
        {
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

    ServerUnaryReactor *
    Configure( CallbackServerContext *context, const Config *request, ConfigResult *reply ) override;

    ServerUnaryReactor *
    ImagePlease( CallbackServerContext *context, const ImageRequest *request, Image *reply ) override;

  signals:

    void gotConfigure( configSpec * );

  protected slots:

    void doConfigure( configSpec *cspec );
};

#endif // rtimvServer_hpp
