// scan_subscriber — LaserScan メッセージのサブスクライバー
//
// scan トピックから LiDAR データを受信し、統計情報を表示する例。
// 障害物検知やコストマップ生成で使われるパターン。

#include <algorithm>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/laserscan.hpp"

class ScanSubscriber : public rclcpp::Node {
public:
    ScanSubscriber()
        : Node("scan_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50055"))
    {
        subscription_ = create_subscription<sensor_msgs::msg::LaserScan>(
            "scan", 10,
            std::bind(&ScanSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Scan Subscriber started — listening on 'scan'");
    }

private:
    void callback(const sensor_msgs::msg::LaserScan& msg)
    {
        size_t n = msg.ranges().size();
        if (n == 0) {
            RCLCPP_WARN(get_logger(), "Empty scan received");
            return;
        }

        // 最小・最大・平均距離を計算
        float min_range = msg.range_max();
        float max_range = msg.range_min();
        double sum = 0.0;
        size_t valid = 0;
        for (size_t i = 0; i < n; ++i) {
            float r = msg.ranges()[i];
            if (r >= msg.range_min() && r <= msg.range_max()) {
                if (r < min_range) min_range = r;
                if (r > max_range) max_range = r;
                sum += r;
                valid++;
            }
        }

        RCLCPP_INFO(get_logger(),
                    "Scan [%s]: %zu points, valid=%zu, min=%.2f max=%.2f avg=%.2f",
                    msg.header().frame_id().c_str(),
                    n, valid, min_range, max_range,
                    valid > 0 ? sum / valid : 0.0);
    }

    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ScanSubscriber>());
    rclcpp::shutdown();
    return 0;
}
