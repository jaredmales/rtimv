
#ifndef rtimvServer_hpp
#define rtimvServer_hpp

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <format>

#include "rtimv.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::Status;

using remote_rtimv::Config;
using remote_rtimv::ConfigResult;
using remote_rtimv::ImageRequest;
using remote_rtimv::Image;

using remote_rtimv::rtimv;

#include <mx/app/application.hpp>

#include "rtimvServerThread.hpp"

#define RTIMV_DEBUG_BREADCRUMB

class rtimvServer : public QObject, public mx::app::application, public rtimv::CallbackService
{
    Q_OBJECT

  public:
    struct configSpec
    {
        std::string m_uri;
        std::string m_config;

        configSpec( const std::string uri, const std::string config ) : m_uri( uri ), m_config( config )
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
