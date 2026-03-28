# ros2-grpc-type-converter

**Language:** [English (current)](#) | [日本語](README_JA.md)

---

## 🎯 What Can You Do?

This library automatically converts ROS 2 message type definitions (IDL) into gRPC Protocol Buffers (.proto) and provides a **ROS 2-compatible API** for gRPC communication. Write ROS 2-style C++ code without needing a full ROS 2 installation.

### Key Features

- **IDL to gRPC**: Automatically convert ROS 2 message definitions to Protocol Buffers
- **ROS 2-Compatible API**: Use `rclcpp::Publisher`, `rclcpp::Subscription`, `rclcpp::Service`, etc.
- **Zero ROS 2 Dependency**: No need to install ROS 2; run standalone gRPC applications
- **Multiple Message Types**: Supports 40+ ROS 2 message packages (std_msgs, geometry_msgs, sensor_msgs, nav_msgs, etc.)
- **Service Support**: Full support for ROS 2 services (AddTwoInts, SetBool, etc.)
- **9+ Sample Programs**: Ready-to-use examples for Pub/Sub, Services, and all message types
- **Comprehensive Tests**: 56 automated integration tests

---

## 💡 Why Use This?

### Problems It Solves

1. **ROS 2 Not Available**: Deploy on systems without ROS 2 (embedded, cloud, edge devices)
2. **Cross-Language Communication**: Use standard gRPC protocols with ROS 2 message semantics
3. **Lightweight Footprint**: Single binary, no heavy ROS 2 middleware
4. **Easy Integration**: Familiar ROS 2 API makes migration straightforward
5. **Debuggable**: Protocol Buffers are language-agnostic and easy to inspect

### Use Cases

- Edge devices collecting sensor data from ROS 2 robots
- Cloud services processing robot telemetry
- Heterogeneous systems mixing ROS 2 and non-ROS components
- Testing ROS 2 code without full ROS 2 infrastructure

---

## 🚀 Quick Start

### Installation (2 steps)

```bash
# 1. Build and install gRPC
./install_grpc.sh

# 2. Build and install ROS 2 message libraries (combines Steps 2-4)
./install_library.sh
```

### Build Examples

```bash
cd grpc_example
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Run Examples

**Publisher (Client):**
```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./publisher_node
```

**Subscriber (Server):**
```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./subscriber_node
```

### Run Tests

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./test_all  # 56 automated integration tests
```

---

## 📖 How to Use

### Basic Publisher (Client)

```cpp
#include <rclcpp/rclcpp.hpp>
#include "geometry_msgs/msg/twist.hpp"

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rclcpp::Node>(
        "cmd_vel_publisher",
        rclcpp::NodeOptions().set_connect_address("localhost:50053"));

    auto publisher = node->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

    geometry_msgs::msg::Twist msg;
    msg.linear().x(1.0);
    msg.angular().z(0.5);

    publisher->publish(msg);

    rclcpp::shutdown();
}
```

### Basic Subscriber (Server)

```cpp
#include <rclcpp/rclcpp.hpp>
#include "geometry_msgs/msg/twist.hpp"

class CmdVelSubscriber : public rclcpp::Node {
public:
    CmdVelSubscriber()
        : Node("cmd_vel_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50053"))
    {
        subscription_ = create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            [](const geometry_msgs::msg::Twist& msg) {
                std::cout << "Linear: " << msg.linear().x()
                          << ", Angular: " << msg.angular().z() << std::endl;
            });
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelSubscriber>());
    rclcpp::shutdown();
}
```

### Service Server

```cpp
auto service = node->create_service<example_interfaces::srv::AddTwoInts>(
    "add_two_ints",
    [](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> req,
       std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> res) {
        res->sum(req->a() + req->b());
    });
```

### Service Client

```cpp
auto client = node->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints");
auto request = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
request->a(3);
request->b(5);
auto future = client->async_send_request(request);
auto response = future.get();
std::cout << "Sum: " << response->sum() << std::endl;  // Output: 8
```

---

## 📚 Available Samples

| Sample | Message Type | Port | Description |
| --- | --- | --- | --- |
| `publisher_node` / `subscriber_node` | `tf2_msgs/TFMessage` | 50051 | TF Pub/Sub |
| `cmd_vel_publisher` / `cmd_vel_subscriber` | `geometry_msgs/Twist` | 50053 | Velocity commands |
| `image_publisher` / `image_subscriber` | `sensor_msgs/Image` | 50054 | Image data |
| `imu_publisher` / `imu_subscriber` | `sensor_msgs/Imu` | 50055 | IMU sensor data |
| `joint_state_publisher` / `joint_state_subscriber` | `sensor_msgs/JointState` | 50056 | Joint angles |
| `odom_publisher` / `odom_subscriber` | `nav_msgs/Odometry` | 50057 | Odometry |
| `scan_publisher` / `scan_subscriber` | `sensor_msgs/LaserScan` | 50058 | Laser scans |
| `service_server` / `service_client` | `example_interfaces/AddTwoInts` | 50052 | Integer addition |
| `setbool_server` / `setbool_client` | `std_srvs/SetBool` | 50059 | Bool service |

---

## 🏗️ Architecture

```text
IDL Files (.idl)
      ↓ (convert_idl_to_proto.py)
Proto Files (.proto)
      ↓ (protoc)
Protocol Buffer C++ code (.pb.h, .pb.cc)
      ↓ (make_access_header.py)
ROS 2-Compatible Wrapper Headers (.hpp)
      ↓ (compile_install_ros_msgs.sh)
Shared Libraries (.so)
      ↓
Your gRPC Application
```

### Communication Model

- **Publisher** (Client): Sends messages via unary gRPC RPC (Stub reused for efficiency)
- **Subscription** (Server): Implements gRPC service, processes received messages in callbacks
- **Service** (Server): Implements Request/Response gRPC operations
- **Client** (Client): Sends async requests to services
- **Timer**: Periodically triggers callbacks using WallTimer

---

## 🔧 Components

| File | Purpose |
| --- | --- |
| `convert_idl_to_proto.py` | Convert IDL files to Proto and compile with protoc |
| `make_access_header.py` | Generate ROS 2-compatible C++ wrapper headers from .pb.h |
| `compile_install_ros_msgs.sh` | Compile C++ sources into shared libraries and install |
| `install_grpc.sh` | Build and install gRPC v1.66.1 from source |
| `install_library.sh` | One-shot script combining Steps 2-4 |
| `grpc_example/include/rclcpp/rclcpp.hpp` | **ROS 2-compatible API layer** — Publisher, Subscription, Service, Client, Timer, Node |
| `grpc_example/src/` | Sample programs for all message types |

---

## 📋 Supported Message Types

| Package | Content |
| --- | --- |
| `std_msgs` | Bool, Int32, String, Header, etc. |
| `builtin_interfaces` | Time, Duration |
| `geometry_msgs` | Point, Pose, Transform, Twist, Quaternion, etc. |
| `sensor_msgs` | Image, PointCloud2, LaserScan, Imu, JointState, etc. |
| `nav_msgs` | Odometry, Path, OccupancyGrid, etc. |
| `tf2_msgs` | TFMessage |
| `visualization_msgs` | Marker, MarkerArray, etc. |
| `diagnostic_msgs` | DiagnosticArray, DiagnosticStatus |
| `lifecycle_msgs` | State, Transition, etc. |
| `stereo_msgs` | DisparityImage |
| `shape_msgs` | Mesh, Plane, etc. |
| `trajectory_msgs` | JointTrajectory, etc. |
| `example_interfaces` | AddTwoInts service, etc. |
| `std_srvs` | SetBool, Trigger, Empty services |

---

## 📋 API Reference

### rclcpp::Node

```cpp
auto node = std::make_shared<rclcpp::Node>(
    "node_name",
    rclcpp::NodeOptions()
        .set_server_address("0.0.0.0:50051")      // For subscriptions/services
        .set_connect_address("localhost:50051"));  // For publishers/clients
```

### rclcpp::Publisher\<T\>

```cpp
auto publisher = node->create_publisher<std_msgs::msg::String>("topic", 10);
std_msgs::msg::String msg;
msg.data("Hello");
publisher->publish(msg);
```

### rclcpp::Subscription\<T\>

```cpp
auto subscription = node->create_subscription<std_msgs::msg::String>(
    "topic", 10,
    [](const std_msgs::msg::String& msg) {
        std::cout << msg.data() << std::endl;
    });
```

### rclcpp::WallTimer

```cpp
using namespace std::chrono_literals;
auto timer = node->create_wall_timer(
    100ms,
    []() { std::cout << "Timer fired!" << std::endl; });
```

### Logging

```cpp
auto logger = rclcpp::get_logger("my_logger");
RCLCPP_INFO(logger, "Info: %d", 42);
RCLCPP_WARN(logger, "Warning: %s", "be careful");
RCLCPP_ERROR(logger, "Error: %.2f", 3.14);
RCLCPP_INFO_STREAM(logger, "Stream: " << value);
```

---

## ⚙️ Installation Details

### Prerequisites

- **OS**: Ubuntu 20.04, 22.04, or 24.04 (x86_64)
- **Compiler**: GCC 9+ (C++17 support)
- **CMake**: 3.10+
- **Python**: 3.8+
- **Disk**: ~10GB free space (for gRPC build)
- **Permissions**: sudo access to `/opt/grpc` and `/opt/grpc-libs`

### Step-by-Step Installation

#### Step 1: Clone Repository

```bash
git clone --recursive https://github.com/tatsuyai713/ros2-grpc-type-converter.git
cd ros2-grpc-type-converter
```

If cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

#### Step 2: Install gRPC

```bash
./install_grpc.sh  # Takes 30 min - 2 hours
```

**Install location**: `/opt/grpc`

#### Step 3: One-Shot Library Install

```bash
./install_library.sh
```

This combines Steps 2-4:
- Converts IDL → Proto
- Compiles ROS 2 libraries
- Generates wrapper headers

**Install location**: `/opt/grpc-libs`

#### Step 4: Build Examples

```bash
cd grpc_example
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

---

## 🐛 Troubleshooting

### Library Not Found

```
error while loading shared libraries: libgrpc++.so
```

Set environment variable:

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib:$LD_LIBRARY_PATH
```

### Header Files Not Found

Ensure headers are installed:

```bash
ls /opt/grpc-libs/include/std_msgs/msg/header.hpp
```

### protoc Version Mismatch

**Solution**: Ensure gRPC Step 1 completed successfully:

```bash
/opt/grpc/bin/protoc --version
/opt/grpc/bin/grpc_cpp_plugin --version
```

### Cannot Connect to Server

- Verify server is running first
- Check firewall allows port (e.g., port 50051)
- For remote: server must listen on `0.0.0.0:PORT`

---

## 🧪 Testing

Run the integration test suite (56 tests):

```bash
cd grpc_example/build
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./test_all
```

Expected output:
```
========================================
  ros2-grpc-type-converter Test Suite
========================================
...
========================================
  Results: 56 passed, 0 failed
========================================
```

---

## 📝 Limitations

- **Unidirectional**: Publisher → Subscription is client → server only
- **Byte Arrays**: `byte` and `octet` types convert to `uint32` arrays (4x memory)
- **QoS**: ROS 2 QoS policies not enforced (accepted for API compatibility)
- **ROS 2 Coexistence**: Library names conflict; cannot run with ROS 2
- **Discovery**: No automatic DDS discovery; requires explicit address
- **Same Type Limit**: One Subscription/Service per message type per Node

---

## 📄 License

MIT License

---

## 🔗 Related Links

- [ROS 2 Documentation](https://docs.ros.org/en/humble/)
- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)

---

**Last Updated**: 2026-03-28
**gRPC Version**: v1.66.1
**Status**: ✅ Actively Maintained
