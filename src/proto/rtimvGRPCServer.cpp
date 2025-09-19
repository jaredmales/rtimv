#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <format>

#include "rtimv.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using remote_rtimv::rtimv;
using remote_rtimv::Config;
using remote_rtimv::ConfigResult;


// Logic and data behind the server's behavior.
class rtimvServiceImpl final : public rtimv::Service
{
    Status Configure( ServerContext* context,
                      const Config* config,
                      ConfigResult* result) override
    {
        std::cout << "Got: " << config->file() << " from: " << context->peer() << '\n';
        result->set_result(0);
        return Status::OK;
    }
};

void RunServer(uint16_t port)
{
  at some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();std::string server_address = std::format("0.0.0.0:{}", port);
  rtimvServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note th
}

int main(int argc, char** argv)
{
  RunServer(7000);
  return 0;
}
