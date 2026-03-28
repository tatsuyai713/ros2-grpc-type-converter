// service_client — ROS 2 style gRPC Service Client example
//
// Demonstrates creating a service client using the rclcpp-compatible API.
// Sends AddTwoInts requests periodically using a timer.

#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include "example_interfaces/srv/add_two_ints.hpp"  // Generated service header

using namespace std::chrono_literals;

class AddTwoIntsClient : public rclcpp::Node {
public:
    AddTwoIntsClient()
        : Node("add_two_ints_client",
               rclcpp::NodeOptions().set_connect_address("localhost:50052"))
    {
        client_ = create_client<example_interfaces::srv::AddTwoInts>(
            "add_two_ints");

        timer_ = create_wall_timer(
            2s, std::bind(&AddTwoIntsClient::send_request, this));

        RCLCPP_INFO(get_logger(), "AddTwoInts service client started");
    }

private:
    void send_request()
    {
        auto request =
            std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
        request->a(count_);
        request->b(count_ * 2);

        RCLCPP_INFO(get_logger(), "Sending request: a=%d b=%d", count_,
                    count_ * 2);

        auto future = client_->async_send_request(request);

        // Wait for the result (blocking, but in a timer callback)
        try {
            auto response = future.get();
            RCLCPP_INFO(get_logger(), "Result: sum=%ld",
                        static_cast<int64_t>(response->sum()));
        } catch (const std::exception& e) {
            RCLCPP_ERROR(get_logger(), "Service call failed: %s", e.what());
        }

        count_++;
    }

    rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_ = 1;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<AddTwoIntsClient>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
