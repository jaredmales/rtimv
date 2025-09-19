
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "rtimv.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using remote_rtimv::rtimv;
using remote_rtimv::Config;
using remote_rtimv::ConfigResult;

class rtimvClient
{
 public:
  rtimvClient(std::shared_ptr<Channel> channel)
      : stub_(rtimv::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  int Configure(const std::string& file)
  {
    // Data we are sending to the server.
    Config request;
    request.set_file(file);

    // Container for the data we expect from the server.
    ConfigResult result;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Configure(&context, request, &result);

    // Act upon its status.
    if (status.ok()) {
      return result.result();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

 private:
  std::unique_ptr<rtimv::Stub> stub_;
};

int main(int argc, char** argv)
{

    std::string target_str= "localhost:7000";

    std::string file = argv[1];

    // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  rtimvClient rtimvC(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  int result = rtimvC.Configure(file);
  std::cout << "rtimvC received: " << result << std::endl;
  return 0;
}
