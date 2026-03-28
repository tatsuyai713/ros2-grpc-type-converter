// odom_publisher — Odometry メッセージのパブリッシャー
//
// ロボットのオドメトリ (位置・速度推定) を odom トピックに送信する例。
// 自律移動ロボットの状態推定ノードで典型的に使われるパターン。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "nav_msgs/msg/odometry.hpp"

using namespace std::chrono_literals;

class OdomPublisher : public rclcpp::Node {
public:
    OdomPublisher()
        : Node("odom_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50054")),
          x_(0.0), y_(0.0), theta_(0.0), count_(0)
    {
        publisher_ = create_publisher<nav_msgs::msg::Odometry>("odom", 10);
        timer_ = create_wall_timer(
            50ms, std::bind(&OdomPublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "Odom Publisher started — topic 'odom' at 20Hz");
    }

private:
    void timer_callback()
    {
        double dt = 0.05;
        double v = 0.3;
        double omega = 0.1;

        theta_ += omega * dt;
        x_ += v * std::cos(theta_) * dt;
        y_ += v * std::sin(theta_) * dt;

        nav_msgs::msg::Odometry msg;

        // Header
        msg.header().frame_id("odom");
        msg.header().stamp().sec(count_ / 20);
        msg.header().stamp().nanosec((count_ % 20) * 50000000);
        msg.child_frame_id() = "base_link";

        // Pose
        msg.pose().pose().position().x(x_);
        msg.pose().pose().position().y(y_);
        msg.pose().pose().position().z(0.0);
        msg.pose().pose().orientation().x(0.0);
        msg.pose().pose().orientation().y(0.0);
        msg.pose().pose().orientation().z(std::sin(theta_ / 2));
        msg.pose().pose().orientation().w(std::cos(theta_ / 2));

        // Twist
        msg.twist().twist().linear().x(v);
        msg.twist().twist().linear().y(0.0);
        msg.twist().twist().linear().z(0.0);
        msg.twist().twist().angular().x(0.0);
        msg.twist().twist().angular().y(0.0);
        msg.twist().twist().angular().z(omega);

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published odom: pos=(%.3f, %.3f) theta=%.2f",
                    x_, y_, theta_);
        count_++;
    }

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr publisher_;
    rclcpp::WallTimer::SharedPtr timer_;
    double x_, y_, theta_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomPublisher>());
    rclcpp::shutdown();
    return 0;
}
