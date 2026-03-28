// imu_publisher — Imu メッセージのパブリッシャー
//
// IMU (加速度・角速度・姿勢) データを imu/data トピックに送信する例。
// ロボットの姿勢推定やセンサーフュージョンで使われるパターン。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/imu.hpp"

using namespace std::chrono_literals;

class ImuPublisher : public rclcpp::Node {
public:
    ImuPublisher()
        : Node("imu_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50056")),
          count_(0)
    {
        publisher_ = create_publisher<sensor_msgs::msg::Imu>("imu/data", 10);
        timer_ = create_wall_timer(
            10ms, std::bind(&ImuPublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "IMU Publisher started — topic 'imu/data' at 100Hz");
    }

private:
    void timer_callback()
    {
        sensor_msgs::msg::Imu msg;

        double t = count_ * 0.01;  // 100Hz

        // Header
        msg.header().frame_id("imu_link");
        msg.header().stamp().sec(static_cast<int32_t>(t));
        msg.header().stamp().nanosec(static_cast<uint32_t>(
            (t - static_cast<int>(t)) * 1e9));

        // 姿勢 (クォータニオン) — ゆっくり揺れるシミュレーション
        double roll = 0.01 * std::sin(t * 0.5);
        double pitch = 0.02 * std::sin(t * 0.3);
        double yaw = t * 0.1;
        // 簡易クォータニオン生成 (小角度近似)
        double cy = std::cos(yaw * 0.5), sy = std::sin(yaw * 0.5);
        double cp = std::cos(pitch * 0.5), sp = std::sin(pitch * 0.5);
        double cr = std::cos(roll * 0.5), sr = std::sin(roll * 0.5);
        msg.orientation().x(sr * cp * cy - cr * sp * sy);
        msg.orientation().y(cr * sp * cy + sr * cp * sy);
        msg.orientation().z(cr * cp * sy - sr * sp * cy);
        msg.orientation().w(cr * cp * cy + sr * sp * sy);

        // 角速度 (rad/s)
        msg.angular_velocity().x(0.01 * std::cos(t * 0.5) * 0.5);
        msg.angular_velocity().y(0.02 * std::cos(t * 0.3) * 0.3);
        msg.angular_velocity().z(0.1);

        // 線形加速度 (m/s²) — 重力 + 振動
        msg.linear_acceleration().x(0.02 * std::sin(t * 2.0));
        msg.linear_acceleration().y(0.01 * std::cos(t * 1.5));
        msg.linear_acceleration().z(9.81 + 0.05 * std::sin(t * 3.0));

        publisher_->publish(msg);

        if (count_ % 100 == 0) {
            RCLCPP_INFO(get_logger(),
                        "Published IMU #%d: accel_z=%.2f gyro_z=%.2f",
                        count_,
                        static_cast<double>(msg.linear_acceleration().z()),
                        static_cast<double>(msg.angular_velocity().z()));
        }
        count_++;
    }

    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr publisher_;
    rclcpp::WallTimer::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImuPublisher>());
    rclcpp::shutdown();
    return 0;
}
