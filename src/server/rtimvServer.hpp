
#ifndef rtimvServer_hpp
#define rtimvServer_hpp


#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <format>

#include "proto/rtimv.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using remote_rtimv::rtimv;
using remote_rtimv::Config;
using remote_rtimv::ConfigResult;

#include <mx/app/application.hpp>

#include "rtimvServerThread.hpp"


#define RTIMV_DEBUG_BREADCRUMB

class rtimvServer : public QObject, public mx::app::application, public rtimv::Service
{
    Q_OBJECT

  public:
    rtimvServer( int argc, char **argv, QObject *Parent = nullptr );

    ~rtimvServer();

    virtual void setupConfig();

    virtual void loadConfig();


    void startServer();

    protected:

    QThread * m_serverThread {nullptr};

    Status Configure( ServerContext* context,
                      const Config* config,
                      ConfigResult* result) override;

    signals:

    void gotConfigure(std::string * configFile);

    protected slots:

    void doConfigure(std::string * configFile);

};


#endif // rtimvServer_hpp
