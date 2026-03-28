// odom_subscriber — Odometry メッセージのサブスクライバー
//
// odom トピックからオドメトリデータを受信して表示する例。
// ナビゲーションスタックやロボット状態監視で使われるパターン。

#include <rclcpp/rclcpp.hpp>
#include "nav_msgs/msg/odometry.hpp"

class OdomSubscriber : public rclcpp::Node {
public:
    OdomSubscriber()
        : Node("odom_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50054"))
    {
        subscription_ = create_subscription<nav_msgs::msg::Odometry>(
            "odom", 10,
            std::bind(&OdomSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Odom Subscriber started — listening on 'odom'");
    }

private:
    void callback(const nav_msgs::msg::Odometry& msg)
    {
        RCLCPP_INFO(get_logger(),
                    "Odom [%s→%s] pos=(%.3f, %.3f, %.3f) vel=(%.2f, %.2f)",
                    msg.header().frame_id().c_str(),
                    msg.child_frame_id().c_str(),
                    static_cast<double>(msg.pose().pose().position().x()),
                    static_cast<double>(msg.pose().pose().position().y()),
                    static_cast<double>(msg.pose().pose().position().z()),
                    static_cast<double>(msg.twist().twist().linear().x()),
                    static_cast<double>(msg.twist().twist().angular().z()));
    }

    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomSubscriber>());
    rclcpp::shutdown();
    return 0;
}
