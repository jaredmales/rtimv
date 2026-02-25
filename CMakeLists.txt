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

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_rtimv_default_systemd_unit_dir "${CMAKE_INSTALL_PREFIX}/lib/systemd/system")
pkg_check_modules(SYSTEMD QUIET systemd)
if(SYSTEMD_FOUND)
    pkg_get_variable(_rtimv_systemd_unit_dir systemd systemdsystemunitdir)
    if(_rtimv_systemd_unit_dir)
        set(_rtimv_default_systemd_unit_dir "${_rtimv_systemd_unit_dir}")
    endif()
endif()

############################################
# Documentation
############################################

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    set(RTIMV_DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/rtimv-dox.in
                   ${CMAKE_CURRENT_BINARY_DIR}/rtimv-dox
                   @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/rtimv-dox
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
else()
    message(STATUS "Doxygen not found; 'docs' target will not be available.")
endif()

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

pkg_check_modules(LIBZMQ QUIET IMPORTED_TARGET libzmq)



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
                             src/common/rtimvFilters.cpp
                             src/common/images/shmimImage.cpp
                             src/common/images/fitsImage.cpp
                             src/common/images/fitsDirectory.cpp)

if(LIBZMQ_FOUND)
    list(APPEND commonsrcs src/common/images/mzmqImage.cpp)
endif()

add_library(commonlib OBJECT ${commonsrcs})

target_compile_options(commonlib PRIVATE -DRTIMV_BASE=rtimvBase -DRTIMV_BASE_INCLUDE="rtimvBase.hpp")

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

if(LIBZMQ_FOUND)
    message(STATUS "enabling milkzmq support (libzmq found)")
    target_compile_definitions(commonlib PUBLIC RTIMV_MZMQ_ENABLED)
    target_link_libraries(commonlib PUBLIC -lxrif)
    target_link_libraries(commonlib PUBLIC PkgConfig::LIBZMQ)
else()
    message(STATUS "disabling milkzmq support (libzmq not found)")
endif()


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
# Progobuf for rtimvServer & rtimvClient
############################################

find_package(Protobuf)
#pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc)
pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)

############################################
# rtimvServer
############################################

if(Protobuf_FOUND AND GRPCPP_FOUND)

option(RTIMV_INSTALL_SYSTEMD_UNIT "Install a systemd unit for rtimvServer" ON)
set(RTIMV_SERVER_SYSTEMD_USER "rtimv" CACHE STRING "User account to run rtimvServer under systemd")
set(RTIMV_SERVER_SYSTEMD_GROUP "rtimv" CACHE STRING "Group account to run rtimvServer under systemd")
set(RTIMV_SERVER_SYSTEMD_ENVIRONMENT_FILE "/etc/default/rtimvServer" CACHE FILEPATH "systemd EnvironmentFile for rtimvServer service")
set(RTIMV_SERVER_SYSTEMD_ARGS "" CACHE STRING "Additional arguments appended to ExecStart for rtimvServer service")
set(RTIMV_SERVER_SYSTEMD_RESTART "on-failure" CACHE STRING "systemd Restart= policy for rtimvServer")
set(RTIMV_SERVER_SYSTEMD_RESTART_SEC "2s" CACHE STRING "systemd RestartSec= for rtimvServer")
set(RTIMV_SERVER_SYSTEMD_UNIT_DIR "${_rtimv_default_systemd_unit_dir}" CACHE PATH "Install directory for rtimvServer systemd unit")

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

install (TARGETS rtimvServer DESTINATION bin)

if(RTIMV_INSTALL_SYSTEMD_UNIT)
    configure_file(systemd/rtimvServer.service.in
                   ${CMAKE_CURRENT_BINARY_DIR}/rtimvServer.service
                   @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rtimvServer.service
            DESTINATION ${RTIMV_SERVER_SYSTEMD_UNIT_DIR})
endif()

endif() #(Protobuf_FOUND AND GRPCPP_FOUND)

############################################
# rtimvClient
#
# This does not use commonlib, as it needs to change the RTIMV_BASE
############################################

if(Protobuf_FOUND AND GRPCPP_FOUND )

    add_executable(rtimvClient src/server/rtimvClientBase.cpp
                               src/server/rtimvClientMain.cpp
                               src/common/rtimvBaseObject.cpp
                               src/proto/rtimv.proto
                               src/gui/rtimvMainWindow.cpp
              src/gui/rtimvControlPanel.cpp
              src/gui/rtimvStats.cpp
              src/gui/cubeCtrl.cpp
              res/rtimv.qrc)

    target_compile_options(rtimvClient PRIVATE -DRTIMV_BASE=rtimvClientBase -DRTIMV_BASE_INCLUDE="rtimvClientBase.hpp" -DRTIMV_GRPC)

    target_include_directories(rtimvClient PRIVATE src/librtimv
                                                   src/common
                                                   src/server
                                                   src/gui
                                                   ${CMAKE_CURRENT_BINARY_DIR}/proto )

    make_directory(${CMAKE_CURRENT_BINARY_DIR}/proto)
    protobuf_generate(TARGET rtimvClient APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS src/proto/rtimv.proto)
    protobuf_generate(TARGET rtimvClient LANGUAGE grpc PLUGIN "protoc-gen-grpc=/usr/bin/grpc_cpp_plugin" GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS src/proto/rtimv.proto)

    if(NOT(MXLIB_HAS_MILK EQUAL -1))
        target_compile_definitions(rtimvClient PUBLIC MXLIB_MILK)
    endif()

    if(NOT(ISIO_HAS_CUDA EQUAL -1))
        target_compile_definitions(rtimvClient PUBLIC HAVE_CUDA)
    endif()

    target_link_libraries(rtimvClient PRIVATE rtimv-shared)
    target_link_libraries(rtimvClient PRIVATE protobuf::libprotobuf)
    target_link_libraries(rtimvClient PRIVATE PkgConfig::GRPCPP)
    target_link_libraries(rtimvClient PRIVATE -lgrpc++_reflection)
    target_link_libraries(rtimvClient PUBLIC Qt5::Widgets)
    target_link_libraries(rtimvClient PUBLIC PkgConfig::MXLIB)

    install (TARGETS rtimvClient DESTINATION bin)

endif() #(Protobuf_FOUND AND GRPCPP_FOUND)
