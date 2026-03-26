// subscriber_node — ROS 2 style gRPC Subscriber example
//
// This example demonstrates how to create a Subscriber node using
// the rclcpp-compatible API.  It listens for TFMessage messages on
// the "tf" topic and prints the received transforms.

#include <rclcpp/rclcpp.hpp>
#include "tf2_msgs/msg/tfmessage.hpp"

class TFSubscriber : public rclcpp::Node {
public:
    TFSubscriber()
        : Node("tf_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50051"))
    {
        subscription_ = create_subscription<tf2_msgs::msg::TFMessage>(
            "tf", 10,
            std::bind(&TFSubscriber::tf_callback, this,
                      std::placeholders::_1));

        RCLCPP_INFO(get_logger(), "TF Subscriber started — waiting for messages on topic 'tf'");
    }

private:
    void tf_callback(const tf2_msgs::msg::TFMessage& msg)
    {
        RCLCPP_INFO(get_logger(), "--- Received TFMessage (%zu transforms) ---",
                    msg.transforms().size());

        for (size_t i = 0; i < msg.transforms().size(); ++i) {
            auto& t = msg.transforms()[i];
            RCLCPP_INFO(get_logger(),
                        "[%zu] frame_id=%s child=%s stamp=(%d, %d) "
                        "translation=(%.2f, %.2f, %.2f)",
                        i,
                        t.header().frame_id().c_str(),
                        t.child_frame_id().c_str(),
                        static_cast<int>(t.header().stamp().sec()),
                        static_cast<int>(t.header().stamp().nanosec()),
                        static_cast<double>(t.transform().translation().x()),
                        static_cast<double>(t.transform().translation().y()),
                        static_cast<double>(t.transform().translation().z()));
        }
    }

    rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<TFSubscriber>();
    RCLCPP_INFO(node->get_logger(), "Starting subscriber on 0.0.0.0:50051...");

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
