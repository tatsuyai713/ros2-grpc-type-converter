cmake_minimum_required(VERSION 3.10)

project(grpc_example)

# C++17を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# リンカ設定
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

# gRPC / Protobuf のインストールパス
set(GRPC_INSTALL_DIR "/opt/grpc")
set(GRPC_LIBS_DIR "/opt/grpc-libs")

set(CMAKE_PREFIX_PATH ${GRPC_INSTALL_DIR} ${CMAKE_PREFIX_PATH})
include(../cmake/common.cmake)

# gRPCとProtobufの検索
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# ヘッダーパスを指定
include_directories(
    ${GRPC_INSTALL_DIR}/include
    ${GRPC_LIBS_DIR}/include
    ${GRPC_LIBS_DIR}/include/std_msgs/msg
    ${GRPC_LIBS_DIR}/include/builtin_interfaces/msg
    ${GRPC_LIBS_DIR}/include/geometry_msgs/msg
    ${GRPC_LIBS_DIR}/include/sensor_msgs/msg
    ${GRPC_LIBS_DIR}/include/tf2_msgs/msg
    ${GRPC_LIBS_DIR}/include/nav_msgs/msg
    ${GRPC_LIBS_DIR}/include/visualization_msgs/msg
    ${GRPC_LIBS_DIR}/include/example_interfaces/srv
    ${GRPC_LIBS_DIR}/include/std_srvs/srv
    ${Protobuf_INCLUDE_DIRS}
    src
)

link_directories(${GRPC_INSTALL_DIR}/lib ${GRPC_LIBS_DIR}/lib)

# ROS 2メッセージライブラリのリスト
set(ROS_MSG_LIBS
    ${GRPC_LIBS_DIR}/lib/libstd_msgs.so
    ${GRPC_LIBS_DIR}/lib/libbuiltin_interfaces.so
    ${GRPC_LIBS_DIR}/lib/libgeometry_msgs.so
    ${GRPC_LIBS_DIR}/lib/libsensor_msgs.so
    ${GRPC_LIBS_DIR}/lib/libtf2_msgs.so
    ${GRPC_LIBS_DIR}/lib/libexample_interfaces.so
)

# 共通のリンクライブラリ
set(COMMON_LIBS
    gRPC::grpc++
    ${Protobuf_LIBRARIES}
    ${ROS_MSG_LIBS}
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    pthread
)

# --- Publisher (client) ---
add_executable(publisher_node src/client.cpp)
target_link_libraries(publisher_node ${COMMON_LIBS})

# --- Subscriber (server) ---
add_executable(subscriber_node src/server.cpp)
target_link_libraries(subscriber_node ${COMMON_LIBS})

# --- Service server example ---
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/service_server.cpp)
    add_executable(service_server src/service_server.cpp)
    target_link_libraries(service_server ${COMMON_LIBS})
endif()

# --- Service client example ---
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/service_client.cpp)
    add_executable(service_client src/service_client.cpp)
    target_link_libraries(service_client ${COMMON_LIBS})
endif()

# Legacy names (backward compat)
add_executable(client src/client.cpp)
target_link_libraries(client ${COMMON_LIBS})

add_executable(server src/server.cpp)
target_link_libraries(server ${COMMON_LIBS})

# --- Test executable ---
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/test_all.cpp)
    add_executable(test_all src/test_all.cpp)
    target_link_libraries(test_all ${COMMON_LIBS})
endif()
