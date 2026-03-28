// scan_publisher — LaserScan メッセージのパブリッシャー
//
// 2D LiDAR スキャンデータを scan トピックに送信する例。
// SLAM やナビゲーションで使われる典型的なセンサーデータ。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/laser_scan.hpp"

using namespace std::chrono_literals;

class ScanPublisher : public rclcpp::Node {
public:
    ScanPublisher()
        : Node("scan_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50055")),
          count_(0)
    {
        publisher_ = create_publisher<sensor_msgs::msg::LaserScan>("scan", 10);
        timer_ = create_wall_timer(
            100ms, std::bind(&ScanPublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "Scan Publisher started — topic 'scan' at 10Hz");
    }

private:
    void timer_callback()
    {
        sensor_msgs::msg::LaserScan msg;

        // Header
        msg.header().frame_id("laser_link");
        msg.header().stamp().sec(count_ / 10);
        msg.header().stamp().nanosec((count_ % 10) * 100000000);

        // スキャンパラメータ (360度、1度刻み)
        int num_readings = 360;
        msg.angle_min(-M_PI);
        msg.angle_max(M_PI);
        msg.angle_increment(2.0 * M_PI / num_readings);
        msg.time_increment(0.0);
        msg.scan_time(0.1f);
        msg.range_min(0.1f);
        msg.range_max(30.0f);

        // 擬似距離データ: 壁に囲まれた部屋をシミュレート
        msg.ranges().resize(num_readings);
        msg.intensities().resize(num_readings);
        for (int i = 0; i < num_readings; ++i) {
            double angle = -M_PI + i * 2.0 * M_PI / num_readings;
            // 矩形の部屋 (5m x 4m) を模擬
            double dx = std::abs(std::cos(angle)) > 0.01 ? 2.5 / std::abs(std::cos(angle)) : 30.0;
            double dy = std::abs(std::sin(angle)) > 0.01 ? 2.0 / std::abs(std::sin(angle)) : 30.0;
            double range = std::min({dx, dy, 30.0});
            // ノイズを加える
            range += 0.01 * (count_ % 5);
            msg.ranges()[i] = static_cast<float>(range);
            msg.intensities()[i] = static_cast<float>(100.0 + (i % 50));
        }

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published scan #%d (%d points)", count_, num_readings);
        count_++;
    }

    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ScanPublisher>());
    rclcpp::shutdown();
    return 0;
}
