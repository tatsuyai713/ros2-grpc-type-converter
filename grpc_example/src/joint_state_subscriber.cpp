// joint_state_subscriber — JointState メッセージのサブスクライバー
//
// joint_states トピックから関節状態を受信し表示する例。
// ロボットアームの状態監視や制御フィードバックで使われるパターン。

#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/jointstate.hpp"

class JointStateSubscriber : public rclcpp::Node {
public:
    JointStateSubscriber()
        : Node("joint_state_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50057"))
    {
        subscription_ = create_subscription<sensor_msgs::msg::JointState>(
            "joint_states", 10,
            std::bind(&JointStateSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "JointState Subscriber started — listening on 'joint_states'");
    }

private:
    void callback(const sensor_msgs::msg::JointState& msg)
    {
        size_t n = msg.name().size();
        RCLCPP_INFO(get_logger(), "JointState received: %zu joints", n);

        for (size_t i = 0; i < n; ++i) {
            double pos = (i < msg.position().size()) ? msg.position()[i] : 0.0;
            double vel = (i < msg.velocity().size()) ? msg.velocity()[i] : 0.0;
            double eff = (i < msg.effort().size()) ? msg.effort()[i] : 0.0;

            RCLCPP_INFO(get_logger(),
                        "  [%s] pos=%.3f rad  vel=%.3f rad/s  effort=%.1f Nm",
                        msg.name()[i].c_str(), pos, vel, eff);
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JointStateSubscriber>());
    rclcpp::shutdown();
    return 0;
}
