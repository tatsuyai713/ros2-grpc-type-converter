// grpc_rcl.hpp — Backward-compatible wrapper
//
// This header is DEPRECATED. Use <rclcpp/rclcpp.hpp> instead.
// It is kept only to avoid breaking existing code.

#ifndef GRPC_NODE_HPP_
#define GRPC_NODE_HPP_

#pragma message("grpc_rcl.hpp is deprecated. Please use <rclcpp/rclcpp.hpp> instead.")

#include "rclcpp/rclcpp.hpp"

namespace grpc_rcl {

// Lifecycle
inline void init(int& argc, char**& argv) {
    rclcpp::init(argc, argv);
}

inline void shutdown() {
    rclcpp::shutdown();
}

// Re-export types under grpc_rcl namespace for backward compat
template <typename T>
using Publisher = rclcpp::Publisher<T>;

template <typename T>
using Subscription = rclcpp::Subscription<T>;

class Node {
public:
    explicit Node(const std::string& address)
        : node_(std::make_shared<rclcpp::Node>(
              "grpc_rcl_node",
              // Auto-detect server vs connect address
              (address.find("0.0.0.0") == 0 || address.find("[::]") == 0)
                  ? rclcpp::NodeOptions().set_server_address(address)
                  : rclcpp::NodeOptions().set_connect_address(address))),
          address_(address) {}

    template <typename T>
    std::shared_ptr<rclcpp::Publisher<T>> create_publisher(
        const std::string& topic_name) {
        // For publisher, need connect address
        if (node_->get_name() == "grpc_rcl_node" &&
            (address_.find("0.0.0.0") != 0 && address_.find("[::]") != 0)) {
            return node_->create_publisher<T>(topic_name);
        }
        // If address looks like a server address, create a channel to it
        auto channel = grpc::CreateChannel(
            address_, grpc::InsecureChannelCredentials());
        return std::make_shared<rclcpp::Publisher<T>>(channel, topic_name);
    }

    template <typename T>
    std::shared_ptr<rclcpp::Subscription<T>> create_subscription(
        const std::string& topic_name,
        std::function<void(const T&)> callback) {
        return node_->create_subscription<T>(topic_name, 10,
                                             std::move(callback));
    }

    // Legacy API: also accept non-const callback
    template <typename T>
    std::shared_ptr<rclcpp::Subscription<T>> create_subscription(
        const std::string& topic_name, std::function<void(T&)> callback) {
        return node_->create_subscription<T>(
            topic_name, 10,
            [cb = std::move(callback)](const T& msg) {
                T mutable_msg;
                mutable_msg.get_grpc()->CopyFrom(*msg.get_grpc());
                cb(mutable_msg);
            });
    }

    void spin() { node_->spin(); }

    ~Node() { node_->shutdown_node(); }

private:
    std::shared_ptr<rclcpp::Node> node_;
    std::string address_;
};

}  // namespace grpc_rcl

#endif  // GRPC_NODE_HPP_
