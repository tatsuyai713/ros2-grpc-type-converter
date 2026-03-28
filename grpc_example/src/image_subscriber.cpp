// image_subscriber — Image メッセージのサブスクライバー
//
// image_raw トピックからカメラ画像を受信し、統計情報を表示する例。
// 画像認識前処理やデータ記録で使われるパターン。

#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/image.hpp"

class ImageSubscriber : public rclcpp::Node {
public:
    ImageSubscriber()
        : Node("image_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50058"))
    {
        subscription_ = create_subscription<sensor_msgs::msg::Image>(
            "image_raw", 10,
            std::bind(&ImageSubscriber::callback, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Image Subscriber started — listening on 'image_raw'");
    }

private:
    void callback(const sensor_msgs::msg::Image& msg)
    {
        size_t n = msg.data().size();
        if (n == 0) {
            RCLCPP_WARN(get_logger(), "Empty image received");
            return;
        }

        // ピクセル値の統計
        uint32_t min_val = 255, max_val = 0;
        double sum = 0.0;
        for (size_t i = 0; i < n; ++i) {
            uint32_t v = msg.data()[i];
            if (v < min_val) min_val = v;
            if (v > max_val) max_val = v;
            sum += v;
        }

        RCLCPP_INFO(get_logger(),
                    "Image [%s]: %ux%u enc=%s size=%zu "
                    "pixel_range=[%u, %u] mean=%.1f",
                    msg.header().frame_id().c_str(),
                    static_cast<unsigned>(msg.width()),
                    static_cast<unsigned>(msg.height()),
                    msg.encoding().c_str(),
                    n, min_val, max_val, sum / n);
    }

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImageSubscriber>());
    rclcpp::shutdown();
    return 0;
}
