
#include <iostream>
#include <chrono>
#include <thread>
#include "grpc_rcl.hpp"
// #include "std_msgs/msg/header.hpp"
// #include "sensor_msgs/msg/image.hpp"
#include "tf2_msgs/msg/tfmessage.hpp"

int main(int argc, char** argv) {
    // Create a gRPC channel
    grpc_rcl::Node node("localhost:50051");
    std::string topic_name = "topic1";

    std::shared_ptr<grpc_rcl::Publisher<tf2_msgs::msg::TFMessage>> publisher = node.create_publisher<tf2_msgs::msg::TFMessage>(topic_name);

    // Create a Header message
    tf2_msgs::msg::TFMessage tfmessage;
    tfmessage.transforms().resize(100);
    tfmessage.transforms()[0].header().frame_id("frame_1");
    tfmessage.transforms()[0].header().stamp().sec(100);
    tfmessage.transforms()[0].header().stamp().nanosec() = 100;
    tfmessage.transforms()[0].child_frame_id() = "child_frame_1";
    tfmessage.transforms()[0].transform().translation().x(1.0);

    geometry_msgs::msg::TransformStamped transform;
    transform.header().frame_id("frame_2");
    transform.header().stamp().sec(200);
    transform.header().stamp().nanosec() = 200;
    transform.child_frame_id() = "child_frame_2";
    transform.transform().translation().x(2.0);
    tfmessage.transforms()[1] = transform;

    // Send the message
    int count = 190;
    while (true) {
        tfmessage.transforms()[0].header().stamp().sec(count);
        count++;
        std::cout << "Message sent: " << tfmessage.transforms()[1].header().frame_id() << std::endl;
        std::cout << "Message sent: " << tfmessage.transforms()[1].header().stamp().sec() << std::endl;
        std::cout << "Message sent: " << tfmessage.transforms()[1].header().stamp().nanosec() << std::endl;

        publisher->publish(tfmessage);
    
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // publisher->publish(header);

    node.spin();

    return 0;
}

// int main(int argc, char** argv) {
//     // Create a gRPC channel
//     grpc_rcl::Node node("localhost:50051");
//     std::string topic_name = "topic1";

//     std::shared_ptr<grpc_rcl::Publisher<sensor_msgs::msg::Image>> publisher = node.create_publisher<sensor_msgs::msg::Image>(topic_name);

//     // Create a Header message
//     sensor_msgs::msg::Image image;
//     image.header().frame_id("frame_1");
//     image.header().stamp().sec() = 10;
//     image.header().stamp().nanosec() = 100;
//     image.height() = 480;
//     image.width() = 640;
//     image.is_bigendian() = 0;
//     image.step() = 1920;
//     image.encoding() = "rgb8";
//     image.data().resize(100);
//     for (int i = 0; i < 100; i++)
//     {
//         image.data()[i] = 99-i;
//     }

//     // Send the message
//     int count = 190;
//     while (true) {
//         image.header().stamp().sec(count);
//         count++;
//         publisher->publish(image);
//         std::cout << "Message sent: " << image.header().frame_id() << std::endl;
//         std::cout << "Message sent: " << image.header().stamp().sec() << std::endl;
    
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
//     // publisher->publish(header);

//     node.spin();

//     return 0;
// }

// int main(int argc, char** argv) {
//     // Create a gRPC channel
//     grpc_rcl::Node node("localhost:50051");
//     std::string topic_name = "topic1";

//     std::shared_ptr<grpc_rcl::Publisher<std_msgs::msg::Header>> publisher = node.create_publisher<std_msgs::msg::Header>(topic_name);

//     // Create a Header message
//     std_msgs::msg::Header header;
//     header.frame_id("frame_1");
//     header.frame_id() = "frame_2";
//     header.stamp().sec(10);
//     header.stamp().nanosec() = 100;

//     // Send the message
//     int count = 190;
//     while (true) {
//         header.stamp().sec(count);
//         count++;
//         publisher->publish(header);
//         std::cout << "Message sent: " << header.frame_id() << std::endl;
//         std::cout << "Message sent: " << header.stamp().sec() << std::endl;
    
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//     }
//     // publisher->publish(header);

//     node.spin();

//     return 0;
// }



// // client/client.cpp

// #include <iostream>
// #include <memory>
// #include <string>

// #include <grpcpp/grpcpp.h>

// using grpc::Channel;
// using grpc::ClientContext;
// using grpc::Status;
// using std_msgs::msg::Header;
// using google::protobuf::Empty;

// int main(int argc, char** argv) {
//     // サーバーのアドレスを指定
//     std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    
//     // Header クラスのインスタンスを作成
//     Header header;
//     header.NewStub(channel);

//     // HeaderGRPCメッセージの設定
//     header.stamp().sec(123456789);
//     header.stamp().nanosec(987654321);
//     header.frame_id("example_frame");

//     // RPC 呼び出し用のコンテキスト
//     ClientContext context;

//     // メッセージの送信
//     Status status = header.send(context);

//     if (status.ok()) {
//         std::cout << "SendGRPC RPC succeeded." << std::endl;
//     } else {
//         std::cout << "SendGRPC RPC failed: " << status.error_message() << std::endl;
//     }

//     return 0;
// }


// #include <iostream>
// #include <memory>
// #include <string>

// #include <grpcpp/grpcpp.h>
// #include "Image.pb.h"
// #include "Image.grpc.pb.h"

// using grpc::Channel;
// using grpc::ClientContext;
// using grpc::Status;
// using sensor_msgs::msg::ImageGRPC;
// using sensor_msgs::msg::ImageGRPCService;
// using google::protobuf::Empty;

// int main(int argc, char** argv) {
//     // サーバーのアドレスを指定
//     std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

//     // サービスのStubを作成
//     std::unique_ptr<ImageGRPCService::Stub> stub = ImageGRPCService::NewStub(channel);

//     // ImageGRPCメッセージの作成と設定
//     ImageGRPC image;
//     auto* header = image.mutable_header();
//     auto* stamp = header->mutable_stamp();
//     stamp->set_sec(123456789);
//     stamp->set_nanosec(987654321);
//     header->set_frame_id("example_frame");

//     image.set_height(480);
//     image.set_width(640);
//     image.set_encoding("rgb8");
//     image.set_is_bigendian(0);
//     image.set_step(1920);
//     for (int i = 0; i < 10; ++i) {
//         image.add_data(i);
//     }

//     // RPC 呼び出し用のコンテキスト
//     ClientContext context;

//     // RPC 呼び出し
//     Empty response;
//     Status status = stub->SendGRPC(&context, image, &response);

//     if (status.ok()) {
//         std::cout << "SendGRPC RPC succeeded." << std::endl;
//     } else {
//         std::cout << "SendGRPC RPC failed: " << status.error_message() << std::endl;
//     }

//     return 0;
// }
