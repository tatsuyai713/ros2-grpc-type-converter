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

# ローカル生成ヘッダー (優先)
set(LOCAL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ros-data-types-for-fastdds/src")

# ヘッダーパスを指定
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${LOCAL_SRC_DIR}
    ${LOCAL_SRC_DIR}/std_msgs/msg
    ${LOCAL_SRC_DIR}/builtin_interfaces/msg
    ${LOCAL_SRC_DIR}/geometry_msgs/msg
    ${LOCAL_SRC_DIR}/sensor_msgs/msg
    ${LOCAL_SRC_DIR}/tf2_msgs/msg
    ${LOCAL_SRC_DIR}/nav_msgs/msg
    ${LOCAL_SRC_DIR}/visualization_msgs/msg
    ${LOCAL_SRC_DIR}/example_interfaces/srv
    ${LOCAL_SRC_DIR}/std_srvs/srv
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
    ${GRPC_LIBS_DIR}/lib/libnav_msgs.so
    ${GRPC_LIBS_DIR}/lib/libexample_interfaces.so
    ${GRPC_LIBS_DIR}/lib/libstd_srvs.so
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

# --- cmd_vel ---
add_executable(cmd_vel_publisher src/cmd_vel_publisher.cpp)
target_link_libraries(cmd_vel_publisher ${COMMON_LIBS})

add_executable(cmd_vel_subscriber src/cmd_vel_subscriber.cpp)
target_link_libraries(cmd_vel_subscriber ${COMMON_LIBS})

# --- image ---
add_executable(image_publisher src/image_publisher.cpp)
target_link_libraries(image_publisher ${COMMON_LIBS})

add_executable(image_subscriber src/image_subscriber.cpp)
target_link_libraries(image_subscriber ${COMMON_LIBS})

# --- imu ---
add_executable(imu_publisher src/imu_publisher.cpp)
target_link_libraries(imu_publisher ${COMMON_LIBS})

add_executable(imu_subscriber src/imu_subscriber.cpp)
target_link_libraries(imu_subscriber ${COMMON_LIBS})

# --- joint_state ---
add_executable(joint_state_publisher src/joint_state_publisher.cpp)
target_link_libraries(joint_state_publisher ${COMMON_LIBS})

add_executable(joint_state_subscriber src/joint_state_subscriber.cpp)
target_link_libraries(joint_state_subscriber ${COMMON_LIBS})

# --- odom ---
add_executable(odom_publisher src/odom_publisher.cpp)
target_link_libraries(odom_publisher ${COMMON_LIBS})

add_executable(odom_subscriber src/odom_subscriber.cpp)
target_link_libraries(odom_subscriber ${COMMON_LIBS})

# --- scan ---
add_executable(scan_publisher src/scan_publisher.cpp)
target_link_libraries(scan_publisher ${COMMON_LIBS})

add_executable(scan_subscriber src/scan_subscriber.cpp)
target_link_libraries(scan_subscriber ${COMMON_LIBS})

# --- setbool service ---
add_executable(setbool_server src/setbool_server.cpp)
target_link_libraries(setbool_server ${COMMON_LIBS})

add_executable(setbool_client src/setbool_client.cpp)
target_link_libraries(setbool_client ${COMMON_LIBS})

# --- test_all ---
add_executable(test_all src/test_all.cpp)
target_link_libraries(test_all ${COMMON_LIBS})
