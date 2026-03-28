// image_publisher — Image メッセージのパブリッシャー
//
// カメラ画像データを image_raw トピックに送信する例。
// コンピュータビジョンや画像処理パイプラインで使われるパターン。

#include <chrono>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/image.hpp"

using namespace std::chrono_literals;

class ImagePublisher : public rclcpp::Node {
public:
    ImagePublisher()
        : Node("image_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50058")),
          count_(0)
    {
        publisher_ = create_publisher<sensor_msgs::msg::Image>("image_raw", 10);
        timer_ = create_wall_timer(
            100ms, std::bind(&ImagePublisher::timer_callback, this));
        RCLCPP_INFO(get_logger(), "Image Publisher started — topic 'image_raw' at 10Hz");
    }

private:
    void timer_callback()
    {
        sensor_msgs::msg::Image msg;

        // Header
        msg.header().frame_id("camera_link");
        msg.header().stamp().sec(count_ / 10);
        msg.header().stamp().nanosec((count_ % 10) * 100000000);

        // 小さなグレースケール画像 (64x48)
        uint32_t width = 64;
        uint32_t height = 48;
        msg.width(width);
        msg.height(height);
        msg.encoding() = "mono8";
        msg.is_bigendian(0);
        msg.step(width);  // 1 byte per pixel

        // 移動するグラデーションパターンを生成
        size_t data_size = width * height;
        msg.data().resize(data_size);
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                double val = 128.0 + 127.0 * std::sin(
                    (x + count_ * 2) * 0.1) * std::cos(y * 0.15);
                msg.data()[y * width + x] = static_cast<uint8_t>(
                    std::max(0.0, std::min(255.0, val)));
            }
        }

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published image #%d (%ux%u %s)",
                    count_, width, height, msg.encoding().c_str());
        count_++;
    }

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
    rclcpp::WallTimer::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImagePublisher>());
    rclcpp::shutdown();
    return 0;
}
