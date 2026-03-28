// test_all.cpp — Automated tests for lwrcl-grpc
//
// Tests:
//   1. Message accessor (unit tests for generated wrapper classes)
//   2. Pub/Sub communication (integration tests for multiple message types)
//   3. Service communication (integration tests for services)
//   4. Node / Timer / Lifecycle API tests

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <atomic>
#include <thread>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_msgs/msg/header.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "builtin_interfaces/msg/time.hpp"
#include "example_interfaces/srv/add_two_ints.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_srvs/srv/set_bool.hpp"

// ============================================================================
// Test utilities
// ============================================================================

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "  FAIL: " << msg << " (" << #cond << ")"             \
                      << std::endl;                                            \
            g_tests_failed++;                                                  \
        } else {                                                               \
            g_tests_passed++;                                                  \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg)                                              \
    do {                                                                       \
        auto _a = (a);                                                         \
        auto _b = (b);                                                         \
        if (_a != _b) {                                                        \
            std::cerr << "  FAIL: " << msg << " (expected=" << _b              \
                      << " got=" << _a << ")" << std::endl;                    \
            g_tests_failed++;                                                  \
        } else {                                                               \
            g_tests_passed++;                                                  \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_NEAR(a, b, tol, msg)                                       \
    do {                                                                       \
        auto _a = (a);                                                         \
        auto _b = (b);                                                         \
        if (std::fabs(_a - _b) > tol) {                                        \
            std::cerr << "  FAIL: " << msg << " (expected=" << _b              \
                      << " got=" << _a << ")" << std::endl;                    \
            g_tests_failed++;                                                  \
        } else {                                                               \
            g_tests_passed++;                                                  \
        }                                                                      \
    } while (0)

// ============================================================================
// Test 1: Message accessor unit tests
// ============================================================================

void test_builtin_time_accessor() {
    std::cout << "[TEST] builtin_interfaces::msg::Time accessor" << std::endl;

    builtin_interfaces::msg::Time t;

    // setter via function call
    t.sec(42);
    t.nanosec(123456);

    // getter via proxy (non-const)
    TEST_ASSERT_EQ(static_cast<int32_t>(t.sec()), 42, "sec getter");
    TEST_ASSERT_EQ(static_cast<uint32_t>(t.nanosec()), 123456u, "nanosec getter");

    // setter via assignment operator
    t.sec() = 100;
    t.nanosec() = 999;
    TEST_ASSERT_EQ(static_cast<int32_t>(t.sec()), 100, "sec assign");
    TEST_ASSERT_EQ(static_cast<uint32_t>(t.nanosec()), 999u, "nanosec assign");

    // const accessor
    const builtin_interfaces::msg::Time& ct = t;
    TEST_ASSERT_EQ(ct.sec(), 100, "const sec");
    TEST_ASSERT_EQ(ct.nanosec(), 999u, "const nanosec");
}

void test_header_accessor() {
    std::cout << "[TEST] std_msgs::msg::Header accessor" << std::endl;

    std_msgs::msg::Header header;

    // string field setter/getter
    header.frame_id("my_frame");
    TEST_ASSERT_EQ(header.frame_id(), std::string("my_frame"), "frame_id setter/getter");

    // string field assignment
    header.frame_id() = "new_frame";
    TEST_ASSERT_EQ(header.frame_id(), std::string("new_frame"), "frame_id assign");

    // sub-message accessor
    header.stamp().sec(1234);
    header.stamp().nanosec(5678);
    TEST_ASSERT_EQ(static_cast<int32_t>(header.stamp().sec()), 1234, "stamp.sec");
    TEST_ASSERT_EQ(static_cast<uint32_t>(header.stamp().nanosec()), 5678u, "stamp.nanosec");

    // const accessor
    const std_msgs::msg::Header& ch = header;
    TEST_ASSERT_EQ(ch.frame_id(), std::string("new_frame"), "const frame_id");
    TEST_ASSERT_EQ(ch.stamp().sec(), 1234, "const stamp.sec");
    TEST_ASSERT_EQ(ch.stamp().nanosec(), 5678u, "const stamp.nanosec");
}

void test_vector3_accessor() {
    std::cout << "[TEST] geometry_msgs::msg::Vector3 accessor" << std::endl;

    geometry_msgs::msg::Vector3 v;
    v.x(1.5);
    v.y(2.5);
    v.z(3.5);

    TEST_ASSERT_NEAR(static_cast<double>(v.x()), 1.5, 1e-9, "x");
    TEST_ASSERT_NEAR(static_cast<double>(v.y()), 2.5, 1e-9, "y");
    TEST_ASSERT_NEAR(static_cast<double>(v.z()), 3.5, 1e-9, "z");

    // const version
    const geometry_msgs::msg::Vector3& cv = v;
    TEST_ASSERT_NEAR(cv.x(), 1.5, 1e-9, "const x");
    TEST_ASSERT_NEAR(cv.y(), 2.5, 1e-9, "const y");
    TEST_ASSERT_NEAR(cv.z(), 3.5, 1e-9, "const z");
}

void test_tfmessage_repeated() {
    std::cout << "[TEST] tf2_msgs::msg::TFMessage repeated field" << std::endl;

    tf2_msgs::msg::TFMessage msg;

    // Initially empty
    TEST_ASSERT_EQ(msg.transforms().size(), (size_t)0, "initial size 0");

    // Resize and set
    msg.transforms().resize(2);
    TEST_ASSERT_EQ(msg.transforms().size(), (size_t)2, "resize to 2");

    msg.transforms()[0].header().frame_id("world");
    msg.transforms()[0].child_frame_id() = "base_link";
    msg.transforms()[0].transform().translation().x(1.0);

    msg.transforms()[1].header().frame_id("base_link");
    msg.transforms()[1].child_frame_id() = "sensor";
    msg.transforms()[1].transform().translation().y(2.0);

    TEST_ASSERT_EQ(msg.transforms()[0].header().frame_id(), std::string("world"), "transform[0] frame_id");
    TEST_ASSERT_EQ(msg.transforms()[0].child_frame_id(), std::string("base_link"), "transform[0] child");
    TEST_ASSERT_NEAR(static_cast<double>(msg.transforms()[0].transform().translation().x()), 1.0, 1e-9, "transform[0] x");

    TEST_ASSERT_EQ(msg.transforms()[1].header().frame_id(), std::string("base_link"), "transform[1] frame_id");
    TEST_ASSERT_EQ(msg.transforms()[1].child_frame_id(), std::string("sensor"), "transform[1] child");
    TEST_ASSERT_NEAR(static_cast<double>(msg.transforms()[1].transform().translation().y()), 2.0, 1e-9, "transform[1] y");
}

void test_message_copy() {
    std::cout << "[TEST] Message copy semantics" << std::endl;

    std_msgs::msg::Header original;
    original.frame_id("original");
    original.stamp().sec(42);

    // Copy assignment
    std_msgs::msg::Header copy;
    copy = original;
    TEST_ASSERT_EQ(copy.frame_id(), std::string("original"), "copy frame_id");
    TEST_ASSERT_EQ(static_cast<int32_t>(copy.stamp().sec()), 42, "copy stamp.sec");

    // Modify copy should not affect original
    copy.frame_id("modified");
    TEST_ASSERT_EQ(original.frame_id(), std::string("original"), "original unchanged after copy mod");
}

// ============================================================================
// Test 2: Pub/Sub integration test
// ============================================================================

void test_pubsub_communication() {
    std::cout << "[TEST] Pub/Sub communication" << std::endl;

    std::atomic<int> received_count{0};
    std::string received_frame_id;
    int32_t received_sec = -1;

    // Create subscriber node
    auto sub_node = std::make_shared<rclcpp::Node>(
        "test_subscriber",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50061"));

    auto subscription = sub_node->create_subscription<tf2_msgs::msg::TFMessage>(
        "test_tf", 10,
        [&](const tf2_msgs::msg::TFMessage& msg) {
            if (msg.transforms().size() > 0) {
                received_frame_id = msg.transforms()[0].header().frame_id();
                received_sec = msg.transforms()[0].header().stamp().sec();
                received_count++;
            }
        });

    // Start server in background thread
    std::thread server_thread([&sub_node]() {
        sub_node->spin_some();
        // Keep server running for a bit
        std::this_thread::sleep_for(std::chrono::seconds(3));
    });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Create publisher node
    auto pub_node = std::make_shared<rclcpp::Node>(
        "test_publisher",
        rclcpp::NodeOptions().set_connect_address("localhost:50061"));

    auto publisher = pub_node->create_publisher<tf2_msgs::msg::TFMessage>("test_tf", 10);

    // Publish a message
    tf2_msgs::msg::TFMessage msg;
    msg.transforms().resize(1);
    msg.transforms()[0].header().frame_id("test_world");
    msg.transforms()[0].header().stamp().sec(777);
    msg.transforms()[0].child_frame_id() = "test_child";
    msg.transforms()[0].transform().translation().x(3.14);

    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Publish a second message
    msg.transforms()[0].header().stamp().sec(888);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Shutdown
    sub_node->shutdown_node();
    server_thread.join();

    TEST_ASSERT(received_count >= 1, "received at least 1 message");
    TEST_ASSERT_EQ(received_frame_id, std::string("test_world"), "received frame_id");
    TEST_ASSERT(received_sec == 777 || received_sec == 888, "received correct sec");
}

// ============================================================================
// Test 3: Service integration test
// ============================================================================

void test_service_communication() {
    std::cout << "[TEST] Service communication (AddTwoInts)" << std::endl;

    // Create service server node
    auto server_node = std::make_shared<rclcpp::Node>(
        "test_service_server",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50062"));

    auto service = server_node->create_service<example_interfaces::srv::AddTwoInts>(
        "test_add",
        [](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> req,
           std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> res) {
            res->sum(static_cast<int64_t>(req->a()) + static_cast<int64_t>(req->b()));
        });

    // Start server
    std::thread server_thread([&server_node]() {
        server_node->spin_some();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Create service client node
    auto client_node = std::make_shared<rclcpp::Node>(
        "test_service_client",
        rclcpp::NodeOptions().set_connect_address("localhost:50062"));

    auto client = client_node->create_client<example_interfaces::srv::AddTwoInts>("test_add");

    // Test 1: 3 + 5 = 8
    {
        auto req = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
        req->a(3);
        req->b(5);
        auto future = client->async_send_request(req);
        auto response = future.get();
        TEST_ASSERT_EQ(static_cast<int64_t>(response->sum()), (int64_t)8, "3+5=8");
    }

    // Test 2: -10 + 25 = 15
    {
        auto req = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
        req->a(-10);
        req->b(25);
        auto future = client->async_send_request(req);
        auto response = future.get();
        TEST_ASSERT_EQ(static_cast<int64_t>(response->sum()), (int64_t)15, "-10+25=15");
    }

    // Test 3: 0 + 0 = 0
    {
        auto req = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
        req->a(0);
        req->b(0);
        auto future = client->async_send_request(req);
        auto response = future.get();
        TEST_ASSERT_EQ(static_cast<int64_t>(response->sum()), (int64_t)0, "0+0=0");
    }

    server_node->shutdown_node();
    server_thread.join();
}

// ============================================================================
// Test 3b: Sample-based message type tests
// ============================================================================

void test_cmd_vel_pubsub() {
    std::cout << "[TEST] Cmd_vel (Twist) Pub/Sub" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_cmd_vel_sub",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50070"));
    std::atomic<bool> received{false};
    auto subscription = server_node->create_subscription<geometry_msgs::msg::Twist>(
        "cmd_vel", 10,
        [&received](const geometry_msgs::msg::Twist& msg) {
            if (msg.linear().x() > 0) received = true;
        });
    std::thread server_thread([&server_node]() {
        for (int i = 0; i < 50; i++) {
            server_node->spin_some();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_cmd_vel_pub",
        rclcpp::NodeOptions().set_connect_address("localhost:50070"));
    auto publisher = client_node->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    geometry_msgs::msg::Twist msg;
    msg.linear().x(1.0);
    msg.angular().z(0.5);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TEST_ASSERT(received, "cmd_vel message received");
    server_node->shutdown_node();
    server_thread.join();
}

void test_imu_pubsub() {
    std::cout << "[TEST] IMU (sensor_msgs::Imu) Pub/Sub" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_imu_sub",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50071"));
    std::atomic<bool> received{false};
    auto subscription = server_node->create_subscription<sensor_msgs::msg::Imu>(
        "imu", 10, [&received](const sensor_msgs::msg::Imu&) { received = true; });
    std::thread server_thread([&server_node]() {
        for (int i = 0; i < 50; i++) {
            server_node->spin_some();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_imu_pub",
        rclcpp::NodeOptions().set_connect_address("localhost:50071"));
    auto publisher = client_node->create_publisher<sensor_msgs::msg::Imu>("imu", 10);
    sensor_msgs::msg::Imu msg;
    msg.linear_acceleration().x(9.81);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TEST_ASSERT(received, "imu message received");
    server_node->shutdown_node();
    server_thread.join();
}

void test_joint_state_pubsub() {
    std::cout << "[TEST] JointState Pub/Sub" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_joint_state_sub",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50072"));
    std::atomic<bool> received{false};
    auto subscription = server_node->create_subscription<sensor_msgs::msg::JointState>(
        "joint_state", 10,
        [&received](const sensor_msgs::msg::JointState& msg) {
            if (msg.name().size() > 0) received = true;
        });
    std::thread server_thread([&server_node]() {
        for (int i = 0; i < 50; i++) {
            server_node->spin_some();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_joint_state_pub",
        rclcpp::NodeOptions().set_connect_address("localhost:50072"));
    auto publisher = client_node->create_publisher<sensor_msgs::msg::JointState>("joint_state", 10);
    sensor_msgs::msg::JointState msg;
    msg.name().push_back("joint_0");
    msg.position().push_back(1.5);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TEST_ASSERT(received, "joint_state message received");
    server_node->shutdown_node();
    server_thread.join();
}

void test_odometry_pubsub() {
    std::cout << "[TEST] Odometry Pub/Sub" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_odom_sub",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50073"));
    std::atomic<bool> received{false};
    auto subscription = server_node->create_subscription<nav_msgs::msg::Odometry>(
        "odom", 10, [&received](const nav_msgs::msg::Odometry&) { received = true; });
    std::thread server_thread([&server_node]() {
        for (int i = 0; i < 50; i++) {
            server_node->spin_some();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_odom_pub",
        rclcpp::NodeOptions().set_connect_address("localhost:50073"));
    auto publisher = client_node->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
    nav_msgs::msg::Odometry msg;
    msg.pose().pose().position().x(1.5);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TEST_ASSERT(received, "odometry message received");
    server_node->shutdown_node();
    server_thread.join();
}

void test_laser_scan_pubsub() {
    std::cout << "[TEST] LaserScan Pub/Sub" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_scan_sub",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50074"));
    std::atomic<bool> received{false};
    auto subscription = server_node->create_subscription<sensor_msgs::msg::LaserScan>(
        "scan", 10, [&received](const sensor_msgs::msg::LaserScan&) { received = true; });
    std::thread server_thread([&server_node]() {
        for (int i = 0; i < 50; i++) {
            server_node->spin_some();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_scan_pub",
        rclcpp::NodeOptions().set_connect_address("localhost:50074"));
    auto publisher = client_node->create_publisher<sensor_msgs::msg::LaserScan>("scan", 10);
    sensor_msgs::msg::LaserScan msg;
    msg.angle_min(-1.57f);
    msg.angle_max(1.57f);
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    TEST_ASSERT(received, "laser_scan message received");
    server_node->shutdown_node();
    server_thread.join();
}

void test_setbool_service() {
    std::cout << "[TEST] SetBool Service" << std::endl;
    auto server_node = std::make_shared<rclcpp::Node>("test_setbool_server",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50075"));
    auto service = server_node->create_service<std_srvs::srv::SetBool>(
        "test_set_bool",
        [](const std::shared_ptr<std_srvs::srv::SetBool::Request> req,
           std::shared_ptr<std_srvs::srv::SetBool::Response> res) {
            res->success(req->data());
        });
    std::thread server_thread([&server_node]() {
        server_node->spin_some();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto client_node = std::make_shared<rclcpp::Node>("test_setbool_client",
        rclcpp::NodeOptions().set_connect_address("localhost:50075"));
    auto client = client_node->create_client<std_srvs::srv::SetBool>("test_set_bool");
    auto req = std::make_shared<std_srvs::srv::SetBool::Request>();
    req->data(true);
    auto future = client->async_send_request(req);
    auto response = future.get();
    TEST_ASSERT_EQ(response->success(), true, "setbool(true) success");
    server_node->shutdown_node();
    server_thread.join();
}

// ============================================================================
// Test 4: Node / Timer / Lifecycle API tests
// ============================================================================

void test_node_api() {
    std::cout << "[TEST] Node API" << std::endl;

    auto node = std::make_shared<rclcpp::Node>(
        "test_node_api",
        rclcpp::NodeOptions()
            .set_server_address("0.0.0.0:50063")
            .set_connect_address("localhost:50063"));

    TEST_ASSERT_EQ(node->get_name(), std::string("test_node_api"), "node name");
    TEST_ASSERT_EQ(node->get_logger().get_name(), std::string("test_node_api"), "logger name");

    node->shutdown_node();
}

void test_lifecycle() {
    std::cout << "[TEST] Lifecycle (init/ok/shutdown)" << std::endl;

    // rclcpp::init was already called in main
    TEST_ASSERT(rclcpp::ok(), "ok() returns true after init");

    // shutdown and re-init for isolated test
    rclcpp::shutdown();
    TEST_ASSERT(!rclcpp::ok(), "ok() returns false after shutdown");

    // Re-init for subsequent tests
    char* argv[] = {nullptr};
    rclcpp::init(0, argv);
    TEST_ASSERT(rclcpp::ok(), "ok() returns true after re-init");
}

void test_timer() {
    std::cout << "[TEST] WallTimer" << std::endl;

    auto node = std::make_shared<rclcpp::Node>(
        "test_timer_node",
        rclcpp::NodeOptions().set_connect_address("localhost:50064"));

    std::atomic<int> timer_count{0};
    auto timer = node->create_wall_timer(
        std::chrono::milliseconds(50),
        [&timer_count]() { timer_count++; });

    // Wait for timer to fire a few times
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    timer->cancel();
    TEST_ASSERT(timer_count >= 3, "timer fired at least 3 times");
    TEST_ASSERT(timer->is_canceled(), "timer is canceled");

    node->shutdown_node();
}

void test_qos() {
    std::cout << "[TEST] QoS" << std::endl;

    rclcpp::QoS qos(10);
    TEST_ASSERT_EQ(qos.depth(), (size_t)10, "QoS depth");

    rclcpp::QoS qos2(1);
    TEST_ASSERT_EQ(qos2.depth(), (size_t)1, "QoS depth 1");
}

void test_logger() {
    std::cout << "[TEST] Logger" << std::endl;

    auto logger = rclcpp::get_logger("test_logger");
    TEST_ASSERT_EQ(logger.get_name(), std::string("test_logger"), "logger name");

    // These should not crash — visual verification only
    RCLCPP_INFO(logger, "Test info: %d", 42);
    RCLCPP_WARN(logger, "Test warn: %s", "warning");
    RCLCPP_ERROR(logger, "Test error: %.2f", 3.14);
    RCLCPP_INFO_STREAM(logger, "Stream test: " << 123);
}

void test_rate() {
    std::cout << "[TEST] Rate" << std::endl;

    rclcpp::Rate rate(10.0);  // 10 Hz
    auto start = std::chrono::steady_clock::now();
    rate.sleep();
    rate.sleep();
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    // 2 sleeps at 10Hz = ~200ms, allow ±100ms tolerance
    TEST_ASSERT(ms >= 100 && ms <= 400, "Rate(10Hz) 2 sleeps ~200ms");
}

void test_node_options() {
    std::cout << "[TEST] NodeOptions" << std::endl;

    rclcpp::NodeOptions opts;
    opts.set_server_address("0.0.0.0:12345")
        .set_connect_address("remote:54321")
        .set_allow_remote(false);

    TEST_ASSERT_EQ(opts.server_address(), std::string("0.0.0.0:12345"), "server_address");
    TEST_ASSERT_EQ(opts.connect_address(), std::string("remote:54321"), "connect_address");
    TEST_ASSERT_EQ(opts.allow_remote(), false, "allow_remote");
}

// ============================================================================
// Test 5: New API tests (Time, Duration, Node::now(), copy constructor regression)
// ============================================================================

void test_duration_api() {
    std::cout << "[TEST] rclcpp::Duration" << std::endl;

    rclcpp::Duration d1(1000000000LL);  // 1 second in nanoseconds
    TEST_ASSERT_EQ(d1.nanoseconds(), (int64_t)1000000000LL, "Duration ns");
    TEST_ASSERT_NEAR(d1.seconds(), 1.0, 1e-9, "Duration sec");

    rclcpp::Duration d2(2, 500000000u);  // 2.5 seconds
    TEST_ASSERT_NEAR(d2.seconds(), 2.5, 1e-9, "Duration(2,500000000) sec");

    // Arithmetic
    auto d3 = d1 + d2;
    TEST_ASSERT_NEAR(d3.seconds(), 3.5, 1e-9, "Duration add");
    auto d4 = d2 - d1;
    TEST_ASSERT_NEAR(d4.seconds(), 1.5, 1e-9, "Duration sub");

    // Comparison
    TEST_ASSERT(d1 < d2, "Duration d1 < d2");
    TEST_ASSERT(d2 > d1, "Duration d2 > d1");
    TEST_ASSERT(d1 == rclcpp::Duration(1000000000LL), "Duration equality");
    TEST_ASSERT(d1 != d2, "Duration inequality");
    TEST_ASSERT(d1 <= d2, "Duration d1 <= d2");
    TEST_ASSERT(d2 >= d1, "Duration d2 >= d1");
}

void test_time_api() {
    std::cout << "[TEST] rclcpp::Time" << std::endl;

    rclcpp::Time t1(10, 500000000u);  // 10.5 seconds
    TEST_ASSERT_NEAR(t1.seconds(), 10.5, 1e-9, "Time(10,500000000) sec");
    TEST_ASSERT_EQ(t1.nanoseconds(), (int64_t)10500000000LL, "Time ns");

    rclcpp::Time t2(12, 0);  // 12.0 seconds
    TEST_ASSERT_NEAR(t2.seconds(), 12.0, 1e-9, "Time(12,0) sec");

    // Time - Time = Duration
    auto diff = t2 - t1;
    TEST_ASSERT_NEAR(diff.seconds(), 1.5, 1e-9, "Time diff");

    // Time + Duration = Time
    rclcpp::Duration d(2000000000LL);  // 2 sec
    auto t3 = t1 + d;
    TEST_ASSERT_NEAR(t3.seconds(), 12.5, 1e-9, "Time + Duration");

    // Time - Duration = Time
    auto t4 = t2 - d;
    TEST_ASSERT_NEAR(t4.seconds(), 10.0, 1e-9, "Time - Duration");

    // Comparison
    TEST_ASSERT(t1 < t2, "Time t1 < t2");
    TEST_ASSERT(t2 > t1, "Time t2 > t1");
    TEST_ASSERT(t1 != t2, "Time inequality");
}

void test_node_now() {
    std::cout << "[TEST] Node::now()" << std::endl;

    auto node = std::make_shared<rclcpp::Node>(
        "test_now_node",
        rclcpp::NodeOptions().set_connect_address("localhost:50076"));

    auto before = std::chrono::system_clock::now();
    rclcpp::Time t = node->now();
    auto after = std::chrono::system_clock::now();

    auto before_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        before.time_since_epoch()).count();
    auto after_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        after.time_since_epoch()).count();

    TEST_ASSERT(t.nanoseconds() >= before_ns, "now() >= before");
    TEST_ASSERT(t.nanoseconds() <= after_ns, "now() <= after");
    TEST_ASSERT(t.seconds() > 0.0, "now() positive");

    node->shutdown_node();
}

void test_copy_constructor_regression() {
    std::cout << "[TEST] Copy constructor regression (deep copy)" << std::endl;

    // Test with Header (has sub-message stamp)
    {
        std_msgs::msg::Header h1;
        h1.frame_id("test_frame");
        h1.stamp().sec(100);
        h1.stamp().nanosec(200);

        std_msgs::msg::Header h2(h1);  // Copy construct
        TEST_ASSERT_EQ(h2.frame_id(), std::string("test_frame"), "copy ctor frame_id");
        TEST_ASSERT_EQ(static_cast<int32_t>(h2.stamp().sec()), 100, "copy ctor stamp.sec");
        TEST_ASSERT_EQ(static_cast<uint32_t>(h2.stamp().nanosec()), 200u, "copy ctor stamp.nanosec");

        // Modify copy - original must be unaffected
        h2.frame_id("modified");
        h2.stamp().sec(999);
        TEST_ASSERT_EQ(h1.frame_id(), std::string("test_frame"), "original unchanged after copy mod");
        TEST_ASSERT_EQ(static_cast<int32_t>(h1.stamp().sec()), 100, "original stamp unchanged");
    }

    // Test with Image (has bytes field)
    {
        sensor_msgs::msg::Image img1;
        img1.width(640);
        img1.height(480);
        img1.encoding() = "rgb8";
        img1.data().resize(10);
        for (size_t i = 0; i < 10; ++i) {
            img1.data()[i] = static_cast<uint8_t>(i);
        }

        sensor_msgs::msg::Image img2(img1);  // Copy construct
        TEST_ASSERT_EQ(static_cast<uint32_t>(img2.width()), 640u, "image copy width");
        TEST_ASSERT_EQ(img2.data().size(), (size_t)10, "image copy data size");
        TEST_ASSERT_EQ(static_cast<uint8_t>(img2.data()[5]), (uint8_t)5, "image copy data[5]");

        // Modify copy - original must be unaffected
        img2.data()[5] = 99;
        TEST_ASSERT_EQ(static_cast<uint8_t>(img1.data()[5]), (uint8_t)5, "original image data unchanged");
    }
}

void test_spin_until_future_complete() {
    std::cout << "[TEST] spin_until_future_complete" << std::endl;

    // Test with AddTwoInts service
    auto server_node = std::make_shared<rclcpp::Node>(
        "test_spin_future_server",
        rclcpp::NodeOptions().set_server_address("0.0.0.0:50077"));

    auto service = server_node->create_service<example_interfaces::srv::AddTwoInts>(
        "test_add_future",
        [](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> req,
           std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> res) {
            res->sum(static_cast<int64_t>(req->a()) + static_cast<int64_t>(req->b()));
        });

    std::thread server_thread([&server_node]() {
        server_node->spin_some();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto client_node = std::make_shared<rclcpp::Node>(
        "test_spin_future_client",
        rclcpp::NodeOptions().set_connect_address("localhost:50077"));

    auto client = client_node->create_client<example_interfaces::srv::AddTwoInts>("test_add_future");

    auto req = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
    req->a(7);
    req->b(3);
    auto future = client->async_send_request(req);

    // Use spin_until_future_complete
    rclcpp::spin_until_future_complete(client_node, future);

    auto response = future.get();
    TEST_ASSERT_EQ(static_cast<int64_t>(response->sum()), (int64_t)10, "spin_until_future 7+3=10");

    server_node->shutdown_node();
    server_thread.join();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "  lwrcl-grpc Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    rclcpp::init(argc, argv);

    // --- Unit tests: message accessors ---
    std::cout << std::endl << "--- Message Accessor Tests ---" << std::endl;
    test_builtin_time_accessor();
    test_header_accessor();
    test_vector3_accessor();
    test_tfmessage_repeated();
    test_message_copy();

    // --- Unit tests: API ---
    std::cout << std::endl << "--- API Tests ---" << std::endl;
    test_node_options();
    test_qos();
    test_logger();
    test_rate();
    test_lifecycle();
    test_node_api();
    test_timer();

    // --- New API tests ---
    std::cout << std::endl << "--- New API Tests ---" << std::endl;
    test_duration_api();
    test_time_api();
    test_node_now();
    test_copy_constructor_regression();

    // --- Integration tests: communication ---
    std::cout << std::endl << "--- Integration Tests ---" << std::endl;
    test_pubsub_communication();
    test_service_communication();
    test_cmd_vel_pubsub();
    test_imu_pubsub();
    test_joint_state_pubsub();
    test_odometry_pubsub();
    test_laser_scan_pubsub();
    test_setbool_service();
    test_spin_until_future_complete();

    // --- Summary ---
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << g_tests_passed << " passed, "
              << g_tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    rclcpp::shutdown();

    return g_tests_failed > 0 ? 1 : 0;
}
