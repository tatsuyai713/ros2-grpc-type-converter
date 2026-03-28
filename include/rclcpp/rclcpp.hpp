// Copyright 2024 lwrcl-grpc Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCLCPP__RCLCPP_HPP_
#define RCLCPP__RCLCPP_HPP_

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <regex>
#include <condition_variable>
#include <future>
#include <queue>
#include <cassert>
#include <csignal>

// ============================================================================
// rclcpp — ROS 2 Compatible API over gRPC
// ============================================================================

namespace rclcpp {

// ===================== Global State =========================================

namespace detail {

inline std::atomic<bool>& g_initialized() {
    static std::atomic<bool> val{false};
    return val;
}

inline std::atomic<bool>& g_running() {
    static std::atomic<bool> val{false};
    return val;
}

inline std::mutex& g_shutdown_mutex() {
    static std::mutex mtx;
    return mtx;
}

inline std::condition_variable& g_shutdown_cv() {
    static std::condition_variable cv;
    return cv;
}

// Timer CV registry for prompt shutdown notification
inline std::mutex& g_timer_registry_mutex() {
    static std::mutex mtx;
    return mtx;
}

inline std::vector<std::condition_variable*>& g_timer_cvs() {
    static std::vector<std::condition_variable*> cvs;
    return cvs;
}

inline void register_timer_cv(std::condition_variable* cv) {
    std::lock_guard<std::mutex> lk(g_timer_registry_mutex());
    g_timer_cvs().push_back(cv);
}

inline void unregister_timer_cv(std::condition_variable* cv) {
    std::lock_guard<std::mutex> lk(g_timer_registry_mutex());
    auto& cvs = g_timer_cvs();
    cvs.erase(std::remove(cvs.begin(), cvs.end(), cv), cvs.end());
}

inline void notify_all_timers() {
    std::lock_guard<std::mutex> lk(g_timer_registry_mutex());
    for (auto* cv : g_timer_cvs()) {
        cv->notify_all();
    }
}

// Signal handler for graceful shutdown (SIGINT)
inline void signal_handler(int signum) {
    (void)signum;
    g_running().store(false);
    g_shutdown_cv().notify_all();
    notify_all_timers();
}

}  // namespace detail

// ===================== Lifecycle Functions ===================================

inline void init(int argc, char* argv[]) {
    detail::g_initialized().store(true);
    detail::g_running().store(true);
    std::signal(SIGINT, detail::signal_handler);
}

inline bool ok() {
    return detail::g_running().load();
}

inline void shutdown() {
    detail::g_running().store(false);
    detail::g_shutdown_cv().notify_all();
    detail::notify_all_timers();
}

// ===================== QoS (Simplified) =====================================

class QoS {
public:
    explicit QoS(size_t depth) : depth_(depth) {}
    size_t depth() const { return depth_; }
private:
    size_t depth_;
};

// ===================== Logger ===============================================

class Logger {
public:
    Logger() : name_("unknown") {}
    explicit Logger(const std::string& name) : name_(name) {}
    const std::string& get_name() const { return name_; }
private:
    std::string name_;
};

inline Logger get_logger(const std::string& name) {
    return Logger(name);
}

namespace detail {

inline void log_output(const char* level, const char* logger_name,
                       const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_r(&time, &tm_buf);
    char time_str[32];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    fprintf(stdout, "[%s] [%s.%03d] [%s]: %s\n",
            level, time_str, static_cast<int>(ms.count()), logger_name, buf);
    fflush(stdout);
}

}  // namespace detail
}  // namespace rclcpp

// ===================== Logging Macros =======================================

#define RCLCPP_INFO(logger, fmt, ...)                                          \
    rclcpp::detail::log_output("INFO", (logger).get_name().c_str(), fmt,       \
                               ##__VA_ARGS__)

#define RCLCPP_WARN(logger, fmt, ...)                                          \
    rclcpp::detail::log_output("WARN", (logger).get_name().c_str(), fmt,       \
                               ##__VA_ARGS__)

#define RCLCPP_ERROR(logger, fmt, ...)                                         \
    rclcpp::detail::log_output("ERROR", (logger).get_name().c_str(), fmt,      \
                               ##__VA_ARGS__)

#define RCLCPP_DEBUG(logger, fmt, ...)                                         \
    rclcpp::detail::log_output("DEBUG", (logger).get_name().c_str(), fmt,      \
                               ##__VA_ARGS__)

#define RCLCPP_INFO_STREAM(logger, stream_expr)                                \
    do {                                                                       \
        std::ostringstream _rclcpp_ss;                                         \
        _rclcpp_ss << stream_expr;                                             \
        RCLCPP_INFO(logger, "%s", _rclcpp_ss.str().c_str());                   \
    } while (0)

#define RCLCPP_WARN_STREAM(logger, stream_expr)                                \
    do {                                                                       \
        std::ostringstream _rclcpp_ss;                                         \
        _rclcpp_ss << stream_expr;                                             \
        RCLCPP_WARN(logger, "%s", _rclcpp_ss.str().c_str());                   \
    } while (0)

#define RCLCPP_ERROR_STREAM(logger, stream_expr)                               \
    do {                                                                       \
        std::ostringstream _rclcpp_ss;                                         \
        _rclcpp_ss << stream_expr;                                             \
        RCLCPP_ERROR(logger, "%s", _rclcpp_ss.str().c_str());                  \
    } while (0)

namespace rclcpp {

// ===================== Network Utilities ====================================

namespace detail {

inline std::string url_decode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());
    for (size_t i = 0; i < encoded.length(); i++) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int val = 0;
            std::istringstream hex(encoded.substr(i + 1, 2));
            if (hex >> std::hex >> val) {
                decoded += static_cast<char>(val);
                i += 2;
            } else {
                decoded += encoded[i];
            }
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

inline std::string parse_ip(const std::string& peer) {
    std::string decoded_peer = url_decode(peer);
    std::regex ipv6_regex(R"(ipv6:\[(.*)\]:\d+)");
    std::regex ipv4_regex(R"(ipv4:(\d+\.\d+\.\d+\.\d+):\d+)");
    std::smatch match;
    if (std::regex_search(decoded_peer, match, ipv6_regex)) {
        return match[1].str();
    } else if (std::regex_search(decoded_peer, match, ipv4_regex)) {
        return match[1].str();
    }
    return decoded_peer;
}

}  // namespace detail

// ===================== ClientManager (Access Control) =======================

class ClientManager {
public:
    void register_client(const std::string& client_ip,
                         const std::string& topic_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        client_topics_[client_ip].insert(topic_name);
    }

    void allow_all_clients(const std::string& topic_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        allow_all_ = true;
        allowed_topics_.insert(topic_name);
    }

    bool is_client_allowed(const std::string& client_ip,
                           const std::string& topic_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (allow_all_ && allowed_topics_.count(topic_name)) return true;
        auto it = client_topics_.find(client_ip);
        if (it == client_topics_.end()) return false;
        return it->second.count(topic_name) > 0;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::set<std::string>> client_topics_;
    bool allow_all_{false};
    std::set<std::string> allowed_topics_;
};

// ===================== NodeOptions ==========================================

class NodeOptions {
public:
    NodeOptions() = default;

    NodeOptions& set_server_address(const std::string& addr) {
        server_address_ = addr;
        return *this;
    }

    NodeOptions& set_connect_address(const std::string& addr) {
        connect_address_ = addr;
        return *this;
    }

    NodeOptions& set_allow_remote(bool allow) {
        allow_remote_ = allow;
        return *this;
    }

    const std::string& server_address() const { return server_address_; }
    const std::string& connect_address() const { return connect_address_; }
    bool allow_remote() const { return allow_remote_; }

private:
    std::string server_address_;
    std::string connect_address_;
    bool allow_remote_ = true;
};

// ===================== Time / Duration (ROS 2 Compatible) ===================

class Duration {
public:
    Duration() : nanoseconds_(0) {}
    explicit Duration(int64_t nanoseconds) : nanoseconds_(nanoseconds) {}
    Duration(int32_t seconds, uint32_t nanoseconds)
        : nanoseconds_(static_cast<int64_t>(seconds) * 1000000000LL + nanoseconds) {}

    int64_t nanoseconds() const { return nanoseconds_; }
    double seconds() const { return nanoseconds_ / 1e9; }

    Duration operator+(const Duration& rhs) const { return Duration(nanoseconds_ + rhs.nanoseconds_); }
    Duration operator-(const Duration& rhs) const { return Duration(nanoseconds_ - rhs.nanoseconds_); }
    bool operator<(const Duration& rhs) const { return nanoseconds_ < rhs.nanoseconds_; }
    bool operator>(const Duration& rhs) const { return nanoseconds_ > rhs.nanoseconds_; }
    bool operator<=(const Duration& rhs) const { return nanoseconds_ <= rhs.nanoseconds_; }
    bool operator>=(const Duration& rhs) const { return nanoseconds_ >= rhs.nanoseconds_; }
    bool operator==(const Duration& rhs) const { return nanoseconds_ == rhs.nanoseconds_; }
    bool operator!=(const Duration& rhs) const { return nanoseconds_ != rhs.nanoseconds_; }

private:
    int64_t nanoseconds_;
};

class Time {
public:
    Time() : nanoseconds_(0) {}
    explicit Time(int64_t nanoseconds) : nanoseconds_(nanoseconds) {}
    Time(int32_t sec, uint32_t nanosec)
        : nanoseconds_(static_cast<int64_t>(sec) * 1000000000LL + nanosec) {}

    int64_t nanoseconds() const { return nanoseconds_; }
    double seconds() const { return nanoseconds_ / 1e9; }

    Duration operator-(const Time& rhs) const { return Duration(nanoseconds_ - rhs.nanoseconds_); }
    Time operator+(const Duration& d) const { return Time(nanoseconds_ + d.nanoseconds()); }
    Time operator-(const Duration& d) const { return Time(nanoseconds_ - d.nanoseconds()); }
    bool operator<(const Time& rhs) const { return nanoseconds_ < rhs.nanoseconds_; }
    bool operator>(const Time& rhs) const { return nanoseconds_ > rhs.nanoseconds_; }
    bool operator<=(const Time& rhs) const { return nanoseconds_ <= rhs.nanoseconds_; }
    bool operator>=(const Time& rhs) const { return nanoseconds_ >= rhs.nanoseconds_; }
    bool operator==(const Time& rhs) const { return nanoseconds_ == rhs.nanoseconds_; }
    bool operator!=(const Time& rhs) const { return nanoseconds_ != rhs.nanoseconds_; }

private:
    int64_t nanoseconds_;
};

// ===================== WallTimer ============================================

class WallTimer {
public:
    using SharedPtr = std::shared_ptr<WallTimer>;

    WallTimer(std::chrono::nanoseconds period, std::function<void()> callback)
        : period_(period), callback_(std::move(callback)), canceled_(false) {
        detail::register_timer_cv(&timer_cv_);
        thread_ = std::thread([this]() {
            auto next_time = std::chrono::steady_clock::now() + period_;
            while (!canceled_.load() && rclcpp::ok()) {
                {
                    std::unique_lock<std::mutex> lk(timer_mutex_);
                    timer_cv_.wait_until(lk, next_time, [this]() {
                        return canceled_.load() || !rclcpp::ok();
                    });
                }
                if (!canceled_.load() && rclcpp::ok() && callback_) {
                    try {
                        callback_();
                    } catch (const std::exception& e) {
                        std::cerr << "[ERROR] Timer callback threw: " << e.what()
                                  << std::endl;
                    }
                }
                next_time += period_;
                // If we fell behind, skip to next future slot
                auto now = std::chrono::steady_clock::now();
                if (next_time < now) {
                    next_time = now + period_;
                }
            }
        });
    }

    ~WallTimer() {
        cancel();
        if (thread_.joinable()) thread_.join();
        detail::unregister_timer_cv(&timer_cv_);
    }

    // Non-copyable
    WallTimer(const WallTimer&) = delete;
    WallTimer& operator=(const WallTimer&) = delete;

    void cancel() {
        canceled_.store(true);
        timer_cv_.notify_all();
    }
    bool is_canceled() const { return canceled_.load(); }

private:
    std::chrono::nanoseconds period_;
    std::function<void()> callback_;
    std::atomic<bool> canceled_;
    std::mutex timer_mutex_;
    std::condition_variable timer_cv_;
    std::thread thread_;
};

// ===================== Publisher =============================================

template <typename MessageT>
class Publisher {
public:
    using SharedPtr = std::shared_ptr<Publisher<MessageT>>;
    using ConstSharedPtr = std::shared_ptr<const Publisher<MessageT>>;

    Publisher(std::shared_ptr<grpc::Channel> channel,
              const std::string& topic_name)
        : channel_(channel), topic_name_(topic_name) {
        stub_holder_.NewStub(channel_);
    }

    void publish(const MessageT& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        grpc::ClientContext context;
        context.AddMetadata("topic-name", topic_name_);
        // Zero-copy: send msg's grpc data directly without CopyFrom
        grpc::Status status = stub_holder_.send_msg(context, *msg.get_grpc());
        if (!status.ok()) {
            std::cerr << "[WARN] Publish failed on '" << topic_name_
                      << "': " << status.error_message() << std::endl;
        }
    }

    void publish(MessageT& msg) {
        publish(static_cast<const MessageT&>(msg));
    }

    const std::string& get_topic_name() const { return topic_name_; }

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::string topic_name_;
    MessageT stub_holder_;
    std::mutex mutex_;
};

// ===================== Subscription =========================================

template <typename MessageT>
class Subscription final : public MessageT {
public:
    using SharedPtr = std::shared_ptr<Subscription<MessageT>>;
    using ConstSharedPtr = std::shared_ptr<const Subscription<MessageT>>;
    using CallbackType = std::function<void(const MessageT&)>;

    Subscription(const std::string& topic_name, CallbackType callback,
                 bool allow_remote = true)
        : topic_name_(topic_name), callback_(std::move(callback)) {
        if (allow_remote) {
            client_manager_.allow_all_clients(topic_name_);
        } else {
            client_manager_.register_client("::1", topic_name_);
            client_manager_.register_client("127.0.0.1", topic_name_);
        }
    }

    void allow_client(const std::string& client_ip) {
        client_manager_.register_client(client_ip, topic_name_);
    }

    grpc::Status SendGRPC(
        grpc::ServerContext* context,
        const decltype(MessageT::type_)* request,
        google::protobuf::Empty* response) override {
        std::string client_ip = detail::parse_ip(context->peer());

        auto metadata = context->client_metadata();
        auto topic_iter = metadata.find("topic-name");
        if (topic_iter == metadata.end()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "Topic name not provided in metadata");
        }
        std::string topic_name(topic_iter->second.data(),
                               topic_iter->second.length());

        if (!client_manager_.is_client_allowed(client_ip, topic_name)) {
            return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                "Client not allowed for topic: " + topic_name);
        }

        // Zero-copy: wrap the incoming request directly (callback is const&)
        if (callback_) {
            MessageT message(const_cast<decltype(MessageT::type_)*>(request));
            callback_(message);
        }

        return grpc::Status::OK;
    }

    const std::string& get_topic_name() const { return topic_name_; }

private:
    std::string topic_name_;
    CallbackType callback_;
    ClientManager client_manager_;
};

// ===================== Service (Server-Side) ================================

template <typename SrvT>
class Service final : public SrvT::ServiceBase {
public:
    using SharedPtr = std::shared_ptr<Service<SrvT>>;
    using CallbackType = std::function<void(
        const std::shared_ptr<typename SrvT::Request>,
        std::shared_ptr<typename SrvT::Response>)>;

    Service(const std::string& service_name, CallbackType callback,
            bool allow_remote = true)
        : service_name_(service_name), callback_(std::move(callback)) {
        if (allow_remote) {
            client_manager_.allow_all_clients(service_name_);
        } else {
            client_manager_.register_client("::1", service_name_);
            client_manager_.register_client("127.0.0.1", service_name_);
        }
    }

    grpc::Status CallService(
        grpc::ServerContext* context,
        const typename SrvT::GrpcRequestType* request,
        typename SrvT::GrpcResponseType* response) override {
        std::string client_ip = detail::parse_ip(context->peer());

        auto metadata = context->client_metadata();
        auto name_iter = metadata.find("service-name");
        if (name_iter == metadata.end()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "Service name not provided in metadata");
        }
        std::string svc_name(name_iter->second.data(),
                             name_iter->second.length());

        if (!client_manager_.is_client_allowed(client_ip, svc_name)) {
            return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                "Client not allowed for service: " + svc_name);
        }

        if (callback_) {
            // Zero-copy: wrap request directly, write response directly
            auto req = std::make_shared<typename SrvT::Request>(
                const_cast<typename SrvT::GrpcRequestType*>(request));
            auto res = std::make_shared<typename SrvT::Response>(response);
            callback_(req, res);
            // No CopyFrom needed: res wrote directly into 'response'
        }

        return grpc::Status::OK;
    }

    const std::string& get_service_name() const { return service_name_; }

private:
    std::string service_name_;
    CallbackType callback_;
    ClientManager client_manager_;
};

// ===================== Client (Service Client) ==============================

template <typename SrvT>
class Client {
public:
    using SharedPtr = std::shared_ptr<Client<SrvT>>;
    using SharedRequest = std::shared_ptr<typename SrvT::Request>;
    using SharedResponse = std::shared_ptr<typename SrvT::Response>;
    using SharedFutureResponse = std::shared_future<SharedResponse>;

    Client(std::shared_ptr<grpc::Channel> channel,
           const std::string& service_name)
        : channel_(channel), service_name_(service_name) {
        stub_ = SrvT::ServiceStubType::NewStub(channel_);
    }

    SharedFutureResponse async_send_request(SharedRequest request) {
        auto promise = std::make_shared<std::promise<SharedResponse>>();
        auto future = promise->get_future().share();

        std::thread([this, request, promise]() {
            grpc::ClientContext context;
            context.AddMetadata("service-name", service_name_);

            // Allocate owned response directly, let gRPC write into it
            auto owned_response = std::make_shared<typename SrvT::Response>();
            grpc::Status status = stub_->CallService(
                &context, *request->get_grpc(), owned_response->get_grpc());

            if (status.ok()) {
                promise->set_value(owned_response);
            } else {
                promise->set_exception(std::make_exception_ptr(
                    std::runtime_error("Service call failed: " +
                                       status.error_message())));
            }
        }).detach();

        return future;
    }

    bool wait_for_service(
        std::chrono::nanoseconds timeout = std::chrono::seconds(1)) {
        return channel_->WaitForConnected(
            std::chrono::system_clock::now() + timeout);
    }

    const std::string& get_service_name() const { return service_name_; }

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::string service_name_;
    std::unique_ptr<typename SrvT::ServiceStubType::Stub> stub_;
};

// ===================== Rate =================================================

class Rate {
public:
    explicit Rate(double hz)
        : period_(std::chrono::nanoseconds(
              static_cast<int64_t>(1e9 / hz))),
          next_time_(std::chrono::steady_clock::now() + period_) {}

    explicit Rate(std::chrono::nanoseconds period)
        : period_(period),
          next_time_(std::chrono::steady_clock::now() + period_) {}

    void sleep() {
        auto now = std::chrono::steady_clock::now();
        if (next_time_ > now) {
            std::this_thread::sleep_until(next_time_);
        }
        next_time_ += period_;
    }

private:
    std::chrono::nanoseconds period_;
    std::chrono::steady_clock::time_point next_time_;
};

// ===================== Node =================================================

class Node {
public:
    using SharedPtr = std::shared_ptr<Node>;

    explicit Node(const std::string& name,
                  const NodeOptions& options = NodeOptions())
        : name_(name),
          options_(options),
          logger_(name),
          server_built_(false) {
        if (!options_.server_address().empty()) {
            builder_.AddListeningPort(options_.server_address(),
                                      grpc::InsecureServerCredentials());
            has_server_ = true;
        }
        if (!options_.connect_address().empty()) {
            channel_ = grpc::CreateChannel(options_.connect_address(),
                                           grpc::InsecureChannelCredentials());
        }
    }

    virtual ~Node() {
        shutdown_node();
    }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // ----- Publisher --------------------------------------------------------

    template <typename MessageT>
    typename Publisher<MessageT>::SharedPtr create_publisher(
        const std::string& topic_name, size_t qos_depth = 10) {
        (void)qos_depth;
        if (!channel_) {
            throw std::runtime_error(
                "Node '" + name_ +
                "' has no connect_address. Set it via NodeOptions to publish.");
        }
        return std::make_shared<Publisher<MessageT>>(channel_, topic_name);
    }

    template <typename MessageT>
    typename Publisher<MessageT>::SharedPtr create_publisher(
        const std::string& topic_name, const QoS& qos) {
        return create_publisher<MessageT>(topic_name, qos.depth());
    }

    // ----- Subscription -----------------------------------------------------

    template <typename MessageT>
    typename Subscription<MessageT>::SharedPtr create_subscription(
        const std::string& topic_name, size_t qos_depth,
        std::function<void(const MessageT&)> callback) {
        (void)qos_depth;
        if (!has_server_) {
            throw std::runtime_error(
                "Node '" + name_ +
                "' has no server_address. Set it via NodeOptions to subscribe.");
        }
        auto sub = std::make_shared<Subscription<MessageT>>(
            topic_name, std::move(callback), options_.allow_remote());
        builder_.RegisterService(sub.get());
        subscriptions_.push_back(sub);
        return sub;
    }

    template <typename MessageT>
    typename Subscription<MessageT>::SharedPtr create_subscription(
        const std::string& topic_name, const QoS& qos,
        std::function<void(const MessageT&)> callback) {
        return create_subscription<MessageT>(topic_name, qos.depth(),
                                             std::move(callback));
    }

    // ----- Service (server-side) --------------------------------------------

    template <typename SrvT>
    typename Service<SrvT>::SharedPtr create_service(
        const std::string& service_name,
        typename Service<SrvT>::CallbackType callback) {
        if (!has_server_) {
            throw std::runtime_error(
                "Node '" + name_ +
                "' has no server_address. Set it via NodeOptions for services.");
        }
        auto svc = std::make_shared<Service<SrvT>>(
            service_name, std::move(callback), options_.allow_remote());
        builder_.RegisterService(svc.get());
        services_.push_back(svc);
        return svc;
    }

    // ----- Client (service client) ------------------------------------------

    template <typename SrvT>
    typename Client<SrvT>::SharedPtr create_client(
        const std::string& service_name) {
        if (!channel_) {
            throw std::runtime_error(
                "Node '" + name_ +
                "' has no connect_address. Set it via NodeOptions for clients.");
        }
        return std::make_shared<Client<SrvT>>(channel_, service_name);
    }

    // ----- Timer ------------------------------------------------------------

    WallTimer::SharedPtr create_wall_timer(
        std::chrono::nanoseconds period, std::function<void()> callback) {
        auto timer = std::make_shared<WallTimer>(period, std::move(callback));
        timers_.push_back(timer);
        return timer;
    }

    template <typename DurationT>
    WallTimer::SharedPtr create_wall_timer(DurationT period,
                                           std::function<void()> callback) {
        return create_wall_timer(
            std::chrono::duration_cast<std::chrono::nanoseconds>(period),
            std::move(callback));
    }

    // ----- Accessors --------------------------------------------------------

    Logger get_logger() const { return logger_; }
    const std::string& get_name() const { return name_; }

    Time now() const {
        auto tp = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            tp.time_since_epoch()).count();
        return Time(ns);
    }

    // ----- Spin (internal) --------------------------------------------------

    void spin() {
        if (has_server_ && !server_built_) {
            server_ = builder_.BuildAndStart();
            server_built_ = true;
            if (server_) {
                RCLCPP_INFO(logger_, "gRPC server listening on %s",
                            options_.server_address().c_str());
            } else {
                RCLCPP_ERROR(logger_, "Failed to start server on %s",
                             options_.server_address().c_str());
                return;
            }
        }

        if (server_) {
            std::unique_lock<std::mutex> lk(detail::g_shutdown_mutex());
            detail::g_shutdown_cv().wait(lk, []() { return !rclcpp::ok(); });
            server_->Shutdown();
        } else {
            std::unique_lock<std::mutex> lk(detail::g_shutdown_mutex());
            detail::g_shutdown_cv().wait(lk, []() { return !rclcpp::ok(); });
        }
    }

    /// Non-blocking spin: ensure server is started then return immediately.
    void spin_some() {
        if (has_server_ && !server_built_) {
            server_ = builder_.BuildAndStart();
            server_built_ = true;
            if (server_) {
                RCLCPP_INFO(logger_, "gRPC server listening on %s",
                            options_.server_address().c_str());
            }
        }
        // No sleep: gRPC handles callbacks on its own threads.
    }

    void shutdown_node() {
        for (auto& timer : timers_) {
            timer->cancel();
        }
        if (server_) {
            server_->Shutdown();
            server_.reset();
        }
        server_built_ = false;
    }

private:
    std::string name_;
    NodeOptions options_;
    Logger logger_;

    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_;
    bool server_built_;
    bool has_server_{false};

    std::shared_ptr<grpc::Channel> channel_;

    std::vector<std::shared_ptr<void>> subscriptions_;
    std::vector<std::shared_ptr<void>> services_;
    std::vector<WallTimer::SharedPtr> timers_;
};

// ===================== TimerBase (ROS 2 Compatible Alias) ===================

using TimerBase = WallTimer;

// ===================== Free-Standing Spin Functions =========================

inline void spin(Node::SharedPtr node) {
    if (node) {
        node->spin();
    }
}

inline void spin_some(Node::SharedPtr node) {
    if (node) {
        node->spin_some();
    }
}

/// Spin the node until the given future completes.
template <typename FutureT>
void spin_until_future_complete(
    Node::SharedPtr node,
    const FutureT& future,
    std::chrono::nanoseconds timeout = std::chrono::nanoseconds(-1))
{
    if (node) {
        node->spin_some();  // Ensure server is started
    }
    if (timeout.count() < 0) {
        future.wait();
    } else {
        future.wait_for(timeout);
    }
}

// ===================== Executors ============================================

namespace executors {

class SingleThreadedExecutor {
public:
    void add_node(Node::SharedPtr node) { nodes_.push_back(node); }

    void spin() {
        if (nodes_.empty()) return;
        // Start all gRPC servers
        for (auto& node : nodes_) {
            node->spin_some();
        }
        // Block on shutdown signal (single thread)
        std::unique_lock<std::mutex> lk(detail::g_shutdown_mutex());
        detail::g_shutdown_cv().wait(lk, []() { return !rclcpp::ok(); });
        // Shutdown all nodes
        for (auto& node : nodes_) {
            node->shutdown_node();
        }
    }

private:
    std::vector<Node::SharedPtr> nodes_;
};

}  // namespace executors

}  // namespace rclcpp

#endif  // RCLCPP__RCLCPP_HPP_
