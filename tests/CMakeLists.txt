if(NOT (Protobuf_FOUND AND GRPCPP_FOUND))
    return()
endif()

add_executable(rtimvClientBaseAsyncRaceTest
               client/rtimvClientBase_async_race_test.cpp
               ${CMAKE_SOURCE_DIR}/src/server/rtimvClientBase.cpp
               ${CMAKE_SOURCE_DIR}/src/common/rtimvBaseObject.cpp
               ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)

target_compile_options(rtimvClientBaseAsyncRaceTest PRIVATE -DRTIMV_BASE=rtimvClientBase -DRTIMV_BASE_INCLUDE="rtimvClientBase.hpp" -DRTIMV_GRPC)

target_include_directories(rtimvClientBaseAsyncRaceTest PRIVATE ${CMAKE_SOURCE_DIR}/src/librtimv
                                                                 ${CMAKE_SOURCE_DIR}/src/common
                                                                 ${CMAKE_SOURCE_DIR}/src/server
                                                                 ${CMAKE_CURRENT_BINARY_DIR}/proto
                                                                 ${CMAKE_BINARY_DIR}/proto)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/proto)
protobuf_generate(TARGET rtimvClientBaseAsyncRaceTest APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)
protobuf_generate(TARGET rtimvClientBaseAsyncRaceTest LANGUAGE grpc PLUGIN "protoc-gen-grpc=/usr/bin/grpc_cpp_plugin" GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)

if(NOT(MXLIB_HAS_MILK EQUAL -1))
    target_compile_definitions(rtimvClientBaseAsyncRaceTest PUBLIC MXLIB_MILK)
endif()

if(NOT(ISIO_HAS_CUDA EQUAL -1))
    target_compile_definitions(rtimvClientBaseAsyncRaceTest PUBLIC HAVE_CUDA)
endif()

target_link_libraries(rtimvClientBaseAsyncRaceTest PRIVATE rtimv-shared)
target_link_libraries(rtimvClientBaseAsyncRaceTest PRIVATE protobuf::libprotobuf)
target_link_libraries(rtimvClientBaseAsyncRaceTest PRIVATE PkgConfig::GRPCPP)
target_link_libraries(rtimvClientBaseAsyncRaceTest PRIVATE -lgrpc++_reflection)
target_link_libraries(rtimvClientBaseAsyncRaceTest PUBLIC Qt5::Widgets)
target_link_libraries(rtimvClientBaseAsyncRaceTest PUBLIC PkgConfig::MXLIB)

add_test(NAME rtimvClientBase_async_race COMMAND rtimvClientBaseAsyncRaceTest)
set_tests_properties(rtimvClientBase_async_race PROPERTIES TIMEOUT 10)
