// publisher_node — ROS 2 style gRPC Publisher example
//
// This example demonstrates how to create a Publisher node using
// the rclcpp-compatible API.  It publishes TFMessage messages on
// the "tf" topic at ~10 Hz using a WallTimer, just like a
// typical ROS 2 publisher node.

#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include "tf2_msgs/msg/tf_message.hpp"

using namespace std::chrono_literals;

class TFPublisher : public rclcpp::Node {
public:
    TFPublisher()
        : Node("tf_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50051")),
          count_(0)
    {
        publisher_ = create_publisher<tf2_msgs::msg::TFMessage>("tf", 10);

        timer_ = create_wall_timer(
            100ms, std::bind(&TFPublisher::timer_callback, this));

        RCLCPP_INFO(get_logger(), "TF Publisher started — publishing on topic 'tf'");
    }

private:
    void timer_callback()
    {
        tf2_msgs::msg::TFMessage msg;
        msg.transforms().resize(2);

        // First transform
        msg.transforms()[0].header().frame_id("world");
        msg.transforms()[0].header().stamp().sec(count_);
        msg.transforms()[0].header().stamp().nanosec() = 0;
        msg.transforms()[0].child_frame_id() = "base_link";
        msg.transforms()[0].transform().translation().x(1.0);
        msg.transforms()[0].transform().translation().y(0.0);
        msg.transforms()[0].transform().translation().z(0.0);

        // Second transform
        geometry_msgs::msg::TransformStamped transform;
        transform.header().frame_id("base_link");
        transform.header().stamp().sec(count_);
        transform.header().stamp().nanosec() = 500000000;
        transform.child_frame_id() = "sensor_link";
        transform.transform().translation().x(0.5);
        transform.transform().translation().y(0.1);
        transform.transform().translation().z(0.3);
        msg.transforms()[1] = transform;

        publisher_->publish(msg);

        RCLCPP_INFO(get_logger(), "Published TFMessage #%d (2 transforms)", count_);
        count_++;
    }

    rclcpp::Publisher<tf2_msgs::msg::TFMessage>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<TFPublisher>();
    RCLCPP_INFO(node->get_logger(), "Publisher connecting to localhost:50051...");

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
