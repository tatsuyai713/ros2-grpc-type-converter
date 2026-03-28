// joint_state_publisher — JointState メッセージのパブリッシャー
//
// ロボットアームの関節状態 (位置・速度・トルク) を joint_states トピックに送信する例。
// MoveIt やロボット状態表示 (RViz) で使われるパターン。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/joint_state.hpp"

using namespace std::chrono_literals;

class JointStatePublisher : public rclcpp::Node {
public:
    JointStatePublisher()
        : Node("joint_state_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50057")),
          count_(0)
    {
        publisher_ = create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
        timer_ = create_wall_timer(
            50ms, std::bind(&JointStatePublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "JointState Publisher started — topic 'joint_states' at 20Hz");
    }

private:
    void timer_callback()
    {
        sensor_msgs::msg::JointState msg;

        // Header
        msg.header().frame_id("base_link");
        msg.header().stamp().sec(count_ / 20);
        msg.header().stamp().nanosec((count_ % 20) * 50000000);

        double t = count_ * 0.05;

        // 6 自由度ロボットアームの関節
        const int num_joints = 6;
        msg.name().resize(num_joints);
        msg.position().resize(num_joints);
        msg.velocity().resize(num_joints);
        msg.effort().resize(num_joints);

        const char* joint_names[] = {
            "shoulder_pan", "shoulder_lift", "elbow",
            "wrist_1", "wrist_2", "wrist_3"
        };

        for (int i = 0; i < num_joints; ++i) {
            msg.name()[i] = joint_names[i];
            // 正弦波で関節を動かす(各関節に位相差)
            double phase = i * M_PI / 3.0;
            msg.position()[i] = std::sin(t * 0.5 + phase) * (M_PI / 4);
            msg.velocity()[i] = 0.5 * std::cos(t * 0.5 + phase) * (M_PI / 4);
            msg.effort()[i] = 10.0 * std::sin(t * 0.3 + phase);
        }

        publisher_->publish(msg);

        if (count_ % 20 == 0) {
            RCLCPP_INFO(get_logger(),
                        "Published joints: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f] rad",
                        static_cast<double>(msg.position()[0]),
                        static_cast<double>(msg.position()[1]),
                        static_cast<double>(msg.position()[2]),
                        static_cast<double>(msg.position()[3]),
                        static_cast<double>(msg.position()[4]),
                        static_cast<double>(msg.position()[5]));
        }
        count_++;
    }

    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JointStatePublisher>());
    rclcpp::shutdown();
    return 0;
}
