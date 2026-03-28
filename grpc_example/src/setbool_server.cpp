// setbool_server — SetBool サービスサーバー
//
// std_srvs/srv/SetBool サービスの実装例。
// ロボットのモーター有効化/無効化など、真偽値で制御するパターン。

#include <rclcpp/rclcpp.hpp>
#include "std_srvs/srv/set_bool.hpp"

class SetBoolServer : public rclcpp::Node {
public:
    SetBoolServer()
        : Node("setbool_server",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50059")),
          enabled_(false)
    {
        service_ = create_service<std_srvs::srv::SetBool>(
            "enable_motor",
            std::bind(&SetBoolServer::handle_request, this,
                      std::placeholders::_1, std::placeholders::_2));
        RCLCPP_INFO(get_logger(), "SetBool service 'enable_motor' ready");
    }

private:
    void handle_request(
        const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
        std::shared_ptr<std_srvs::srv::SetBool::Response> response)
    {
        bool requested = request->data();
        enabled_ = requested;

        response->success(true);
        if (enabled_) {
            response->message() = "Motor enabled";
        } else {
            response->message() = "Motor disabled";
        }

        RCLCPP_INFO(get_logger(), "Motor %s (request: %s)",
                    enabled_ ? "ENABLED" : "DISABLED",
                    requested ? "true" : "false");
    }

    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr service_;
    bool enabled_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SetBoolServer>());
    rclcpp::shutdown();
    return 0;
}
