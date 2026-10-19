add_executable(rtimvCommandTest librtimv/rtimvCommand_test.cpp)
target_include_directories(rtimvCommandTest PRIVATE ${CMAKE_SOURCE_DIR}/src/librtimv ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(rtimvCommandTest PRIVATE rtimv-shared ${RTIMV_QT_PACKAGE}::Widgets PkgConfig::MXLIB)
add_test(NAME rtimvCommand COMMAND rtimvCommandTest)
set_tests_properties(rtimvCommand PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 10)

add_executable(rtimvFiltersMtfTest
               common/rtimvFilters_mtf_test.cpp
               ${CMAKE_SOURCE_DIR}/src/common/rtimvFilters.cpp)

target_include_directories(rtimvFiltersMtfTest PRIVATE ${CMAKE_SOURCE_DIR}/src/common)

target_link_libraries(rtimvFiltersMtfTest PRIVATE PkgConfig::MXLIB)

add_test(NAME rtimvFilters_mtf COMMAND rtimvFiltersMtfTest)

add_executable(rtimvBaseMaskStatsTest common/rtimvBase_maskStats_test.cpp)
target_include_directories(rtimvBaseMaskStatsTest PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
target_link_libraries(rtimvBaseMaskStatsTest PRIVATE commonlib rtimv-shared)
add_test(NAME rtimvBase_maskStats COMMAND rtimvBaseMaskStatsTest)
set_tests_properties(rtimvBase_maskStats PROPERTIES TIMEOUT 30)

set(maskStatsGuiSources gui/rtimvMaskStats_gui_test.cpp
                        ${CMAKE_SOURCE_DIR}/src/gui/rtimvMainWindow.cpp
                        ${CMAKE_SOURCE_DIR}/src/gui/rtimvControlPanel.cpp
                        ${CMAKE_SOURCE_DIR}/src/gui/rtimvStats.cpp
                        ${CMAKE_SOURCE_DIR}/src/gui/cubeCtrl.cpp
                        ${CMAKE_SOURCE_DIR}/res/rtimv.qrc)

add_executable(rtimvMaskStatsGuiTest ${maskStatsGuiSources})
target_compile_definitions(rtimvMaskStatsGuiTest PRIVATE RTIMV_BASE_INCLUDE="rtimvBase.hpp")
target_include_directories(rtimvMaskStatsGuiTest PRIVATE ${CMAKE_SOURCE_DIR}/src/common
                                                       ${CMAKE_SOURCE_DIR}/src/librtimv
                                                       ${CMAKE_SOURCE_DIR}/src/gui)
target_link_libraries(rtimvMaskStatsGuiTest PRIVATE commonlib rtimv-shared)
add_test(NAME rtimvMaskStats_gui COMMAND rtimvMaskStatsGuiTest)
set_tests_properties(rtimvMaskStats_gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

if(NOT (Protobuf_FOUND AND GRPCPP_FOUND))
    return()
endif()

add_library(rtimvClientBaseTestSupport STATIC
            ${CMAKE_SOURCE_DIR}/src/server/rtimvClientBase.cpp
            ${CMAKE_SOURCE_DIR}/src/common/rtimvBaseObject.cpp
            ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)

target_compile_options(rtimvClientBaseTestSupport PUBLIC -DRTIMV_BASE=rtimvClientBase -DRTIMV_BASE_INCLUDE="rtimvClientBase.hpp" -DRTIMV_GRPC)

target_include_directories(rtimvClientBaseTestSupport PUBLIC ${CMAKE_SOURCE_DIR}/src/librtimv
                                                            ${CMAKE_SOURCE_DIR}/src/common
                                                            ${CMAKE_SOURCE_DIR}/src/server
                                                            ${CMAKE_CURRENT_BINARY_DIR}/proto
                                                            ${CMAKE_BINARY_DIR}/proto)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/proto)
protobuf_generate(TARGET rtimvClientBaseTestSupport APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)
protobuf_generate(TARGET rtimvClientBaseTestSupport LANGUAGE grpc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}" GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc APPEND_PATH PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto PROTOS ${CMAKE_SOURCE_DIR}/src/proto/rtimv.proto)

if(NOT(MXLIB_HAS_MILK EQUAL -1))
    target_compile_definitions(rtimvClientBaseTestSupport PUBLIC MXLIB_MILK)
endif()

if(NOT(ISIO_HAS_CUDA EQUAL -1))
    target_compile_definitions(rtimvClientBaseTestSupport PUBLIC HAVE_CUDA)
endif()

target_link_libraries(rtimvClientBaseTestSupport PUBLIC rtimv-shared
                                                      protobuf::libprotobuf
                                                      PkgConfig::GRPCPP
                                                      -lgrpc++_reflection
                                                      ${RTIMV_QT_PACKAGE}::Widgets
                                                      PkgConfig::MXLIB)

add_executable(rtimvMaskStatsClientGuiTest ${maskStatsGuiSources})
target_include_directories(rtimvMaskStatsClientGuiTest PRIVATE ${CMAKE_SOURCE_DIR}/src/gui)
target_link_libraries(rtimvMaskStatsClientGuiTest PRIVATE rtimvClientBaseTestSupport)
add_dependencies(rtimvMaskStatsClientGuiTest rtimvServer)
add_test(NAME rtimvMaskStats_clientGui COMMAND rtimvMaskStatsClientGuiTest $<TARGET_FILE:rtimvServer>)
set_tests_properties(rtimvMaskStats_clientGui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 45)

add_executable(rtimvClientBaseAsyncRaceTest client/rtimvClientBase_async_race_test.cpp)
add_executable(rtimvClientBaseMaskStatsTest client/rtimvClientBase_maskStats_test.cpp)
target_link_libraries(rtimvClientBaseMaskStatsTest PRIVATE rtimvClientBaseTestSupport)
add_test(NAME rtimvClientBase_maskStats COMMAND rtimvClientBaseMaskStatsTest)
set_tests_properties(rtimvClientBase_maskStats PROPERTIES TIMEOUT 20)

target_link_libraries(rtimvClientBaseAsyncRaceTest PRIVATE rtimvClientBaseTestSupport)

add_test(NAME rtimvClientBase_async_race COMMAND rtimvClientBaseAsyncRaceTest)
set_tests_properties(rtimvClientBase_async_race PROPERTIES TIMEOUT 10)

add_executable(rtimvClientBaseLockOrderTest client/rtimvClientBase_lock_order_test.cpp)
target_link_libraries(rtimvClientBaseLockOrderTest PRIVATE rtimvClientBaseTestSupport)
add_test(NAME rtimvClientBase_lock_order COMMAND rtimvClientBaseLockOrderTest)
set_tests_properties(rtimvClientBase_lock_order PROPERTIES TIMEOUT 10)

add_executable(rtimvClientBaseReconnectTest client/rtimvClientBase_reconnect_test.cpp)
target_link_libraries(rtimvClientBaseReconnectTest PRIVATE rtimvClientBaseTestSupport)
add_test(NAME rtimvClientBase_reconnect COMMAND rtimvClientBaseReconnectTest)
set_tests_properties(rtimvClientBase_reconnect PROPERTIES TIMEOUT 20)
