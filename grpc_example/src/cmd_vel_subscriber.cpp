// cmd_vel_subscriber — Twist メッセージによるロボット速度指令サブスクライバー
//
// cmd_vel トピックから速度指令を受信し表示する例。
// 実際のロボットではモータードライバへの指令変換をここで行う。

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
            std::bind(&CmdVelSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "CmdVel Subscriber started — listening on 'cmd_vel'");
    }

private:
    void callback(const geometry_msgs::msg::Twist& msg)
    {
        RCLCPP_INFO(get_logger(),
                    "Received cmd_vel: linear=(%.2f, %.2f, %.2f) angular=(%.2f, %.2f, %.2f)",
                    static_cast<double>(msg.linear().x()),
                    static_cast<double>(msg.linear().y()),
                    static_cast<double>(msg.linear().z()),
                    static_cast<double>(msg.angular().x()),
                    static_cast<double>(msg.angular().y()),
                    static_cast<double>(msg.angular().z()));
    }

    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelSubscriber>());
    rclcpp::shutdown();
    return 0;
}
