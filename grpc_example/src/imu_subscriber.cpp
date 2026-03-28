// imu_subscriber — Imu メッセージのサブスクライバー
//
// imu/data トピックから IMU データを受信し表示する例。
// EKF や相補フィルタの入力として使われるパターン。

#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/imu.hpp"

class ImuSubscriber : public rclcpp::Node {
public:
    ImuSubscriber()
        : Node("imu_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50056")),
          msg_count_(0)
    {
        subscription_ = create_subscription<sensor_msgs::msg::Imu>(
            "imu/data", 10,
            std::bind(&ImuSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "IMU Subscriber started — listening on 'imu/data'");
    }

private:
    void callback(const sensor_msgs::msg::Imu& msg)
    {
        msg_count_++;
        // 100Hz なので 100 メッセージごとに表示
        if (msg_count_ % 100 != 0) return;

        // クォータニオンからオイラー角を復元
        double qx = msg.orientation().x();
        double qy = msg.orientation().y();
        double qz = msg.orientation().z();
        double qw = msg.orientation().w();

        double sinr = 2.0 * (qw * qx + qy * qz);
        double cosr = 1.0 - 2.0 * (qx * qx + qy * qy);
        double roll = std::atan2(sinr, cosr);

        double sinp = 2.0 * (qw * qy - qz * qx);
        double pitch = std::abs(sinp) >= 1.0
                           ? std::copysign(M_PI / 2, sinp)
                           : std::asin(sinp);

        double siny = 2.0 * (qw * qz + qx * qy);
        double cosy = 1.0 - 2.0 * (qy * qy + qz * qz);
        double yaw = std::atan2(siny, cosy);

        RCLCPP_INFO(get_logger(),
                    "IMU [%s] rpy=(%.3f, %.3f, %.3f) "
                    "gyro=(%.3f, %.3f, %.3f) accel=(%.2f, %.2f, %.2f)",
                    msg.header().frame_id().c_str(),
                    roll, pitch, yaw,
                    static_cast<double>(msg.angular_velocity().x()),
                    static_cast<double>(msg.angular_velocity().y()),
                    static_cast<double>(msg.angular_velocity().z()),
                    static_cast<double>(msg.linear_acceleration().x()),
                    static_cast<double>(msg.linear_acceleration().y()),
                    static_cast<double>(msg.linear_acceleration().z()));
    }

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr subscription_;
    int msg_count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImuSubscriber>());
    rclcpp::shutdown();
    return 0;
}
