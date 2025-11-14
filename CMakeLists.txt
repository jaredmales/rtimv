cmake_minimum_required(VERSION 3.24)

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

include(FindPkgConfig)
include(CheckLanguage)
find_package(PkgConfig REQUIRED)

project(rtimv LANGUAGES CXX
              VERSION "0.1.0"
              DESCRIPTION "A qt-based real-time image viewer for astronomy."
              HOMEPAGE_URL "https://jaredmales.github.io/rtimv-doc/index.html")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

############################################
# Qt setup
############################################

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

############################################
# mxlib setup
############################################

pkg_check_modules(MXLIB REQUIRED IMPORTED_TARGET mxlib)

############################################
# zmq setup
############################################

pkg_check_modules(LIBZMQ REQUIRED IMPORTED_TARGET libzmq)



############################################
# librtimv
############################################
add_library(objlib OBJECT src/librtimv/rtimvGraphicsView.cpp
                          src/librtimv/StretchBox.cpp
                          src/librtimv/StretchCircle.cpp
                          src/librtimv/StretchLine.cpp)

target_link_libraries(objlib PUBLIC Qt5::Widgets)


# shared libraries need PIC
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(rtimv-shared SHARED $<TARGET_OBJECTS:objlib>)
add_library(rtimv-static STATIC $<TARGET_OBJECTS:objlib>)

#This makes it so we have libmxlib.so and libmxlib.a, without the -shared/-static
SET_TARGET_PROPERTIES(rtimv-shared PROPERTIES OUTPUT_NAME rtimv CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(rtimv-static PROPERTIES OUTPUT_NAME rtimv CLEAN_DIRECT_OUTPUT 1)

install (FILES src/librtimv/rtimvInterfaces.hpp
               src/librtimv/rtimvGraphicsView.hpp
               src/librtimv/StretchGraphicsItem.hpp
               src/librtimv/StretchBox.hpp
               src/librtimv/StretchCircle.hpp
               src/librtimv/StretchLine.hpp
               DESTINATION include/rtimv )

install (TARGETS rtimv-shared EXPORT rtimv-sharedTargets DESTINATION lib)
install (TARGETS rtimv-static EXPORT rtimv-staticTargets DESTINATION lib)

install( EXPORT rtimv-sharedTargets
         FILE rtimv-sharedTargets.cmake
         DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/rtimv" )

############################################
# common objects
############################################

set( commonsrcs src/common/rtimvBaseObject.cpp
                             src/common/rtimvBase.cpp
                             src/common/images/shmimImage.cpp
                             src/common/images/mzmqImage.cpp
                             src/common/images/fitsImage.cpp
                             src/common/images/fitsDirectory.cpp)

add_library(commonlib OBJECT ${commonsrcs})

list(FIND MXLIB_CFLAGS "-DMXLIB_MILK" MXLIB_HAS_MILK)

if(NOT(MXLIB_HAS_MILK EQUAL -1))
    message(STATUS "enabling MILK")
    target_compile_definitions(commonlib PUBLIC MXLIB_MILK)
else()
    message(STATUS "not using MILK")
endif()

list(FIND MXLIB_CFLAGS "-DHAVE_CUDA" ISIO_HAS_CUDA)

if(NOT(ISIO_HAS_CUDA EQUAL -1))
    message(STATUS "enabling CUDA for MILK")
    target_compile_definitions(commonlib PUBLIC HAVE_CUDA)
else()
    message(STATUS "not using CUDA for MILK")
endif()

target_link_libraries(commonlib PUBLIC Qt5::Widgets)
target_link_libraries(commonlib PUBLIC PkgConfig::MXLIB)
target_link_libraries(commonlib PUBLIC PkgConfig::LIBZMQ)
target_link_libraries(commonlib PUBLIC -lxrif)


############################################
# rtimv main window
############################################

set(rtimvsrcs src/gui/rtimvMainWindow.cpp
              src/gui/rtimvControlPanel.cpp
              src/gui/rtimvStats.cpp
              src/gui/cubeCtrl.cpp
              src/rtimv/rtimvMain.cpp
              res/rtimv.qrc)

add_executable(rtimv ${rtimvsrcs})

target_compile_options(rtimv PRIVATE -DRTIMV_BASE_INCLUDE="rtimvBase.hpp")

target_include_directories(rtimv PRIVATE src/librtimv
                                         src/common
                                         src/common/images
                                         src/gui
                                         src/rtimv )

target_link_libraries(rtimv PRIVATE commonlib)
target_link_libraries(rtimv PRIVATE rtimv-shared)

install (TARGETS rtimv DESTINATION bin)


############################################
# rtimvServer
############################################

find_package(Protobuf)
#pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc)
pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)

if(Protobuf_FOUND AND GRPCPP_FOUND)

add_executable(rtimvServer src/server/rtimvServer.cpp
                           src/server/rtimvServerThread.cpp
                           src/server/rtimvServerMain.cpp
                           src/proto/rtimv.proto )

target_include_directories(rtimvServer PRIVATE src/librtimv
                                               src/common
                                               src/common/images
                                               src/gui
                                               ${CMAKE_CURRENT_BINARY_DIR}/proto )

make_directory(${CMAKE_CURRENT_BINARY_DIR}/proto)
protobuf_generate(TARGET rtimvServer APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS src/proto/rtimv.proto)
protobuf_generate(TARGET rtimvServer LANGUAGE grpc PLUGIN "protoc-gen-grpc=/usr/bin/grpc_cpp_plugin" GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS src/proto/rtimv.proto)

target_link_libraries(rtimvServer PRIVATE commonlib)
target_link_libraries(rtimvServer PRIVATE rtimv-shared)
target_link_libraries(rtimvServer PRIVATE protobuf::libprotobuf)
target_link_libraries(rtimvServer PRIVATE PkgConfig::GRPCPP)
target_link_libraries(rtimvServer PRIVATE -lgrpc++_reflection)

install (TARGETS rtimv DESTINATION bin)

endif() #(Protobuf_FOUND AND GRPCPP_FOUND)
