protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` rtimv.proto
protoc -I . --cpp_out=. rtimv.proto

sudo apt install libgrpc++-dev
sudo apt  install protobuf-compiler
sudo apt install protobuf-compiler-grpc

g++ -std=c++20 -I/usr/include rtimvGRPCServer.cpp rtimv.grpc.pb.cc rtimv.pb.cc -o rtimvGRPCServer -l grpc++ -l gpr -l absl_synchronization -l protobuf -l grpc++_reflection

g++ -std=c++20 -I/usr/include rtimvGRPCClient.cpp rtimv.grpc.pb.cc rt
imv.pb.cc -o rtimvGRPCClient -l grpc++ -l gpr -l absl_synchronization -l protobuf -l grpc++_reflection

