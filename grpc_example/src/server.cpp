#include "grpc_rcl.hpp"
// #include "std_msgs/msg/header.hpp"
// #include "sensor_msgs/msg/image.hpp"
#include "tf2_msgs/msg/tfmessage.hpp"

void callback(tf2_msgs::msg::TFMessage &tfmessage)
{
    std::cout << "Callback: Processing Header with frame_id: " << tfmessage.transforms()[0].header().frame_id() << std::endl;
    std::cout << "Callback: Processing Header with stamp.sec: " << tfmessage.transforms()[0].header().stamp().sec() << std::endl;
    std::cout << "Callback: Processing Header with stamp.nanosec: " << tfmessage.transforms()[0].header().stamp().nanosec() << std::endl;
    std::cout << "Callback: Processing Header with child_frame_id: " << tfmessage.transforms()[0].child_frame_id() << std::endl;
    std::cout << "Callback: Processing Header with transform.translation.x: " << tfmessage.transforms()[0].transform().translation().x() << std::endl;
    std::cout << "Callback: Processing Header with transform.translation.y: " << tfmessage.transforms()[0].transform().translation().y() << std::endl;
    std::cout << "Callback: Processing Header with transform.translation.z: " << tfmessage.transforms()[0].transform().translation().z() << std::endl;
    // ここにコールバック処理を実装
}

int main(int argc, char **argv)
{
    // Create and start the gRPC server
    std::string topic_name = "topic1";
    grpc_rcl::Node node("0.0.0.0:50051");

    // Create a subscription
    std::shared_ptr<grpc_rcl::Subscription<tf2_msgs::msg::TFMessage>> subscription = node.create_subscription<tf2_msgs::msg::TFMessage>(topic_name, callback);

    node.spin();
    return 0;
}


// void callback(sensor_msgs::msg::Image &image)
// {
//     std::cout << "Callback: Processing Header with frame_id: " << image.header().frame_id() << std::endl;
//     std::cout << "Callback: Processing Header with stamp.sec: " << image.header().stamp().sec() << std::endl;
//     std::cout << "Callback: Processing Header with data: " << image.data()[0] << std::endl;
//     std::cout << "Callback: Processing Header with data: " << image.data()[1] << std::endl;
//     std::cout << "Callback: Processing Header with data: " << image.data()[2] << std::endl;
//     // ここにコールバック処理を実装
// }

// int main(int argc, char **argv)
// {
//     // Create and start the gRPC server
//     std::string topic_name = "topic1";
//     grpc_rcl::Node node("0.0.0.0:50051");

//     // Create a subscription
//     std::shared_ptr<grpc_rcl::Subscription<sensor_msgs::msg::Image>> subscription = node.create_subscription<sensor_msgs::msg::Image>(topic_name, callback);

//     node.spin();
//     return 0;
// }

// void callback(std_msgs::msg::Header &header)
// {
//     std::cout << "Callback: Processing Header with frame_id: " << header.frame_id() << std::endl;
//     std::cout << "Callback: Processing Header with stamp.sec: " << header.stamp().sec() << std::endl;
//     // ここにコールバック処理を実装
// }

// int main(int argc, char **argv)
// {
//     // Create and start the gRPC server
//     std::string topic_name = "topic1";
//     grpc_rcl::Node node("0.0.0.0:50051");

//     // Create a subscription
//     std::shared_ptr<grpc_rcl::Subscription<std_msgs::msg::Header>> subscription = node.create_subscription<std_msgs::msg::Header>(topic_name, callback);

//     node.spin();
//     return 0;
// }


// // server/server.cpp

// #include <iostream>
// #include <memory>
// #include <string>

// #include <grpcpp/grpcpp.h>

// using grpc::Server;
// using grpc::ServerBuilder;
// using grpc::ServerContext;
// using grpc::Status;
// using google::protobuf::Empty;

// // サービスの実装クラス
// class HeaderServiceImpl : public std_msgs::msg::Header {
// public:
//     HeaderServiceImpl() : Header() {}

//     // RPC メソッドのオーバーライド
//     Status SendGRPC(ServerContext* context, const std_msgs::msg::HeaderGRPC* request, Empty* response) override {
//         // HeaderGRPCからHeaderクラスへのラップ
//         std_msgs::msg::Header header(const_cast<std_msgs::msg::HeaderGRPC*>(request));

//         std::cout << "Received HeaderGRPC message:" << std::endl;
//         std::cout << "  stamp.sec: " << header.stamp().sec() << std::endl;
//         std::cout << "  stamp.nanosec: " << header.stamp().nanosec() << std::endl;
//         std::cout << "  frame_id: " << header.frame_id() << std::endl;

//         // 必要な処理をここに実装

//         return Status::OK;
//     }
// };

// // サーバーの起動関数
// void RunServer() {
//     std::string server_address("0.0.0.0:50051");
//     HeaderServiceImpl service;

//     ServerBuilder builder;
//     // サーバーアドレスを指定
//     builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//     // サービスを登録
//     builder.RegisterService(&service);
//     // サーバーを構築
//     std::unique_ptr<Server> server(builder.BuildAndStart());
//     std::cout << "Server listening on " << server_address << std::endl;

//     // サーバーを待機
//     server->Wait();
// }

// int main(int argc, char** argv) {
//     RunServer();
//     return 0;
// }

// #include <iostream>
// #include <memory>
// #include <string>

// #include <grpcpp/grpcpp.h>
// #include "Image.pb.h"
// #include "Image.grpc.pb.h"

// using grpc::Server;
// using grpc::ServerBuilder;
// using grpc::ServerContext;
// using grpc::Status;
// using sensor_msgs::msg::ImageGRPC;
// using sensor_msgs::msg::ImageGRPCService;
// using google::protobuf::Empty;

// // サービスの実装クラス
// class ImageServiceImpl : public ImageGRPCService::Service {
// public:
//     Status SendGRPC(ServerContext* context, const ImageGRPC* request, Empty* response) override {
//         std::cout << "Received ImageGRPC message:" << std::endl;

//         // HeaderGRPCのフィールドを出力
//         const auto& header = request->header();
//         std::cout << "  Header:" << std::endl;
//         std::cout << "    stamp.sec: " << header.stamp().sec() << std::endl;
//         std::cout << "    stamp.nanosec: " << header.stamp().nanosec() << std::endl;
//         std::cout << "    frame_id: " << header.frame_id() << std::endl;

//         // その他のフィールドを出力
//         std::cout << "  Height: " << request->height() << std::endl;
//         std::cout << "  Width: " << request->width() << std::endl;
//         std::cout << "  Encoding: " << request->encoding() << std::endl;
//         std::cout << "  Is BigEndian: " << request->is_bigendian() << std::endl;
//         std::cout << "  Step: " << request->step() << std::endl;

//         std::cout << "  Data:";
//         for (int i = 0; i < request->data_size(); ++i) {
//             std::cout << " " << request->data(i);
//         }
//         std::cout << std::endl;

//         return Status::OK;
//     }
// };

// // サーバーの起動関数
// void RunServer() {
//     std::string server_address("0.0.0.0:50051");
//     ImageServiceImpl service;

//     ServerBuilder builder;
//     builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//     builder.RegisterService(&service);

//     std::unique_ptr<Server> server(builder.BuildAndStart());
//     std::cout << "Server listening on " << server_address << std::endl;

//     server->Wait();
// }

// int main(int argc, char** argv) {
//     RunServer();
//     return 0;
// }
