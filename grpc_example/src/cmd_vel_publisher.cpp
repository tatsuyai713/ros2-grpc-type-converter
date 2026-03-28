// cmd_vel_publisher — Twist メッセージによるロボット速度指令パブリッシャー
//
// ロボットの並進速度・回転速度を cmd_vel トピックに送信する例。
// 典型的な差動駆動ロボットの制御で使われるパターン。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "geometry_msgs/msg/twist.hpp"

using namespace std::chrono_literals;

class CmdVelPublisher : public rclcpp::Node {
public:
    CmdVelPublisher()
        : Node("cmd_vel_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50053")),
          count_(0)
    {
        publisher_ = create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
        timer_ = create_wall_timer(
            200ms, std::bind(&CmdVelPublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "CmdVel Publisher started — topic 'cmd_vel'");
    }

private:
    void timer_callback()
    {
        geometry_msgs::msg::Twist msg;

        // 円を描くような走行指令
        double t = count_ * 0.1;
        msg.linear().x(0.5);                     // 前進 0.5 m/s
        msg.linear().y(0.0);
        msg.linear().z(0.0);
        msg.angular().x(0.0);
        msg.angular().y(0.0);
        msg.angular().z(std::sin(t) * 0.3);      // 左右に振れる旋回

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published cmd_vel: linear.x=%.2f angular.z=%.2f",
                    static_cast<double>(msg.linear().x()),
                    static_cast<double>(msg.angular().z()));
        count_++;
    }

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelPublisher>());
    rclcpp::shutdown();
    return 0;
}
