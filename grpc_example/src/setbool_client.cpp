// setbool_client — SetBool サービスクライアント
//
// std_srvs/srv/SetBool サービスを呼び出す例。
// ON/OFF を交互に切り替えてモーター制御をデモする。

#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include "std_srvs/srv/set_bool.hpp"

using namespace std::chrono_literals;

class SetBoolClient : public rclcpp::Node {
public:
    SetBoolClient()
        : Node("setbool_client",
               rclcpp::NodeOptions().set_connect_address("localhost:50059")),
          state_(false)
    {
        client_ = create_client<std_srvs::srv::SetBool>("enable_motor");
        timer_ = create_wall_timer(
            3s, std::bind(&SetBoolClient::send_request, this));
        RCLCPP_INFO(get_logger(), "SetBool client started — calling 'enable_motor'");
    }

private:
    void send_request()
    {
        state_ = !state_;  // トグル
        auto request = std::make_shared<std_srvs::srv::SetBool::Request>();
        request->data(state_);

        RCLCPP_INFO(get_logger(), "Sending: data=%s", state_ ? "true" : "false");

        auto future = client_->async_send_request(request);
        auto response = future.get();

        RCLCPP_INFO(get_logger(), "Response: success=%s message='%s'",
                    response->success() ? "true" : "false",
                    response->message().c_str());
    }

    rclcpp::Client<std_srvs::srv::SetBool>::SharedPtr client_;
    rclcpp::TimerBase::SharedPtr timer_;
    bool state_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SetBoolClient>());
    rclcpp::shutdown();
    return 0;
}
