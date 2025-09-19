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

target_link_libraries(objlib Qt5::Widgets)


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
add_library(commonlib OBJECT src/common/rtimvBaseObject.cpp
                             src/common/rtimvBase.cpp
                             src/common/images/shmimImage.cpp
                             src/common/images/mzmqImage.cpp
                             src/common/images/fitsImage.cpp
                             src/common/images/fitsDirectory.cpp)

target_link_libraries(commonlib Qt5::Widgets)


############################################
# rtimv main window
############################################

add_executable(rtimv src/gui/rtimvMainWindow.cpp
                     src/gui/rtimvControlPanel.cpp
                     src/gui/rtimvStats.cpp
                     src/gui/cubeCtrl.cpp
                     src/rtimv/rtimvMain.cpp
                     res/rtimv.qrc )

target_include_directories(rtimv PRIVATE src/librtimv
                                         src/common
                                         src/common/images
                                         src/gui )

target_link_libraries(rtimv PUBLIC Qt5::Widgets)
target_link_libraries(rtimv PRIVATE commonlib)
target_link_libraries(rtimv PUBLIC rtimv-shared)
target_link_libraries(rtimv PUBLIC PkgConfig::MXLIB)
target_link_libraries(rtimv PUBLIC PkgConfig::LIBZMQ)
target_link_libraries(rtimv PUBLIC -lxrif)

install (TARGETS rtimv DESTINATION bin)
