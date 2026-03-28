// service_server — ROS 2 style gRPC Service Server example
//
// Demonstrates creating a service server using the rclcpp-compatible API.
// Implements an AddTwoInts service that receives two integers and returns
// their sum.
//
// This example requires the example_interfaces service proto to be
// generated with service support (convert_idl_to_proto.py) and the
// service wrapper header (make_access_header.py).

#include <rclcpp/rclcpp.hpp>
#include "example_interfaces/srv/add_two_ints.hpp"  // Generated service header

class AddTwoIntsServer : public rclcpp::Node {
public:
    AddTwoIntsServer()
        : Node("add_two_ints_server",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50052"))
    {
        service_ = create_service<example_interfaces::srv::AddTwoInts>(
            "add_two_ints",
            std::bind(&AddTwoIntsServer::handle_add_two_ints, this,
                      std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(get_logger(), "AddTwoInts service server ready");
    }

private:
    void handle_add_two_ints(
        const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> request,
        std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> response)
    {
        auto a = static_cast<int64_t>(request->a());
        auto b = static_cast<int64_t>(request->b());
        response->sum(a + b);

        RCLCPP_INFO(get_logger(), "Incoming request: a=%ld b=%ld → sum=%ld",
                    a, b, static_cast<int64_t>(response->sum()));
    }

    rclcpp::Service<example_interfaces::srv::AddTwoInts>::SharedPtr service_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<AddTwoIntsServer>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
