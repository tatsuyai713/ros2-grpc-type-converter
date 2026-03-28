# ros2-grpc-type-converter

ROS 2 のメッセージ型定義 (IDL) を gRPC の Protocol Buffers (.proto) に自動変換し、ROS 2 ライクな API で gRPC 通信を行えるようにするライブラリです。

ROS 2 を使わずに、ROS 2 互換のメッセージ型を gRPC 経由で送受信できます。

> **注意**: 出力されるライブラリ名が ROS 2 の標準ライブラリ名と競合するため、ROS 2 環境と同時に使用することはできません。

## アーキテクチャ

```text
┌─────────────────────────────────────────────────────────────────┐
│                      ビルドパイプライン                           │
│                                                                 │
│  IDL (.idl)  ──→  Proto (.proto)  ──→  C++ gRPC コード (.pb.h) │
│  (ROS 2型定義)   convert_idl_to_proto.py   protoc               │
│                                                                 │
│  .pb.h  ──→  ラッパーヘッダー (.hpp)                              │
│            make_access_header.py                                │
│            (ROS 2スタイルのアクセサ生成)                           │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐     gRPC (TCP)     ┌──────────────┐
│   Client     │ ─────────────────→ │   Server     │
│  Publisher   │   topic metadata   │ Subscription │
│              │   + protobuf msg   │  (callback)  │
└──────────────┘                    └──────────────┘
```

### コンポーネント

| ファイル | 役割 |
| --- | --- |
| `convert_idl_to_proto.py` | IDL ファイルを Proto ファイルに変換し、protoc でコンパイル |
| `make_access_header.py` | protoc 生成の `.pb.h` を解析し、ROS 2 スタイルの C++ ラッパーヘッダーを生成。サービスペアのラッパーも自動生成 |
| `compile_install_ros_msgs.sh` | 変換された C++ ソースを共有ライブラリにコンパイルしインストール |
| `install_grpc.sh` | gRPC v1.66.1 をソースからビルドしインストール |
| `install_library.sh` | Step 2〜4 を一括実行するインストールスクリプト |
| `grpc_example/include/rclcpp/rclcpp.hpp` | **ROS 2 互換 API レイヤー** — Publisher / Subscription / Service / Client / Timer / Node |
| `grpc_example/src/grpc_rcl.hpp` | 後方互換ラッパー（非推奨、rclcpp.hpp の利用を推奨） |

### 通信モデル

- **Publisher** (クライアント側): gRPC のユナリ RPC でメッセージを送信（Stub 再利用で高効率）
- **Subscription** (サーバー側): gRPC サービスとして実装、受信メッセージをコールバックで処理
- **Service** (サーバー側): Request / Response パターンの gRPC ユナリ RPC
- **Client** (クライアント側): Service に対する非同期リクエスト送信
- **Timer**: WallTimer によるコールバックの定期実行
- トピック名・サービス名は gRPC メタデータとして送信
- クライアント IP によるアクセス制御をサポート（デフォルトは全許可）

## 制限事項

- **通信方向**: Publisher → Subscription は クライアント → サーバーの一方向
- **byte 型**: `octet` / `byte` 配列は `uint32` 配列に変換されるため、メモリ使用量が 4 倍になる
- **QoS**: ROS 2 の QoS ポリシーはサポートされない（API 互換性のため引数としては受け取る）
- **ROS 2 非互換**: ライブラリ名の競合により ROS 2 環境との共存不可
- **ディスカバリ**: DDS の自動ディスカバリはなく、明示的なアドレス指定が必要
- **同一メッセージ型制限**: 1つの Node につき同一メッセージ型の Subscription/Service は 1 つまで

## 前提条件

- **OS**: Ubuntu 20.04 / 22.04 / 24.04 (x86_64)
- **コンパイラ**: GCC 9+ (C++17 対応)
- **CMake**: 3.10+
- **Python**: 3.8+
- **ディスク容量**: gRPC ビルドに約 10GB の空き容量が必要
- **権限**: `/opt/grpc` と `/opt/grpc-libs` への書き込みに sudo が必要

## リポジトリのクローン

```bash
git clone --recursive https://github.com/tatsuyai713/ros2-grpc-type-converter.git
cd ros2-grpc-type-converter
```

`--recursive` を付けないでクローンした場合:

```bash
git submodule update --init --recursive
```

## インストール手順

### 一括インストール (推奨)

Step 2〜4 を一括で実行するスクリプトが用意されています。

```bash
# Step 1: gRPC のインストール (初回のみ)
./install_grpc.sh

# Step 2〜4: ライブラリのビルド・インストール
./install_library.sh
```

### 個別ステップでのインストール

#### Step 1: gRPC のインストール

gRPC v1.66.1 をソースからビルドし、`/opt/grpc` にインストールします。

```bash
./install_grpc.sh
```

ビルドには時間がかかります（マシンスペックに依存、30分〜2時間程度）。

**インストール先**: `/opt/grpc`

#### Step 2: IDL から Proto への変換

ROS 2 のメッセージ定義 (IDL) を Protocol Buffers 形式に変換します。

```bash
LD_LIBRARY_PATH=/opt/grpc/lib python3 ./convert_idl_to_proto.py ./ros-data-types-for-fastdds/src/
```

このスクリプトは以下を行います:

1. `.idl` ファイルを `.proto` ファイルに変換
2. protoc を使って C++ ソース (`.pb.h`, `.pb.cc`, `.grpc.pb.h`, `.grpc.pb.cc`) を生成

#### Step 3: ROS 2 メッセージライブラリのコンパイル・インストール

変換された C++ ソースを共有ライブラリにコンパイルします。

```bash
LD_LIBRARY_PATH=/opt/grpc/lib ./compile_install_ros_msgs.sh ./ros-data-types-for-fastdds/src/
```

**インストール先**: `/opt/grpc-libs`

#### Step 4: アクセスヘッダーの生成

ROS 2 スタイルの C++ ラッパーヘッダーを生成し、インストールします。

```bash
python3 ./make_access_header.py ./ros-data-types-for-fastdds/src/
sudo cp -rf ./ros-data-types-for-fastdds/src/* /opt/grpc-libs/include/
```

#### Step 5: サンプルのビルド

```bash
cd grpc_example
mkdir -p build && cd build
cmake ..
make
```

## 使い方

環境変数の設定:

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib:$LD_LIBRARY_PATH
```

### テストの実行

全メッセージ型・サービス型の統合テスト:

```bash
cd grpc_example/build
./test_all
```

### サンプル一覧

以下のサンプルが `grpc_example/build/` に生成されます。各ペアのサーバー側を先に起動してください。

| サンプル | メッセージ型 | ポート | 説明 |
| --- | --- | --- | --- |
| `subscriber_node` / `publisher_node` | `tf2_msgs/TFMessage` | 50051 | TF メッセージの Pub/Sub |
| `cmd_vel_subscriber` / `cmd_vel_publisher` | `geometry_msgs/Twist` | 50053 | 速度指令の Pub/Sub |
| `image_subscriber` / `image_publisher` | `sensor_msgs/Image` | 50054 | 画像データの Pub/Sub |
| `imu_subscriber` / `imu_publisher` | `sensor_msgs/Imu` | 50055 | IMU データの Pub/Sub |
| `joint_state_subscriber` / `joint_state_publisher` | `sensor_msgs/JointState` | 50056 | 関節状態の Pub/Sub |
| `odom_subscriber` / `odom_publisher` | `nav_msgs/Odometry` | 50057 | オドメトリの Pub/Sub |
| `scan_subscriber` / `scan_publisher` | `sensor_msgs/LaserScan` | 50058 | レーザースキャンの Pub/Sub |
| `service_server` / `service_client` | `example_interfaces/AddTwoInts` | 50052 | 整数加算サービス |
| `setbool_server` / `setbool_client` | `std_srvs/SetBool` | 50059 | Bool サービス (モーター ON/OFF) |

### 実行例

**Pub/Sub (TF メッセージ):**

```bash
# ターミナル 1 (サーバー)
./subscriber_node

# ターミナル 2 (クライアント)
./publisher_node
```

**サービス (AddTwoInts):**

```bash
# ターミナル 1
./service_server

# ターミナル 2
./service_client
```

**サービス (SetBool):**

```bash
# ターミナル 1
./setbool_server

# ターミナル 2
./setbool_client
```

## API リファレンス

### rclcpp::Node

ROS 2 互換のノードクラス。`NodeOptions` でサーバーアドレス（Subscription/Service用）とコネクトアドレス（Publisher/Client用）を指定します。

```cpp
#include <rclcpp/rclcpp.hpp>

// Subscriber ノード: サーバーアドレスを指定
auto sub_node = std::make_shared<rclcpp::Node>("my_subscriber",
    rclcpp::NodeOptions().set_server_address("0.0.0.0:50051"));

// Publisher ノード: コネクトアドレスを指定
auto pub_node = std::make_shared<rclcpp::Node>("my_publisher",
    rclcpp::NodeOptions().set_connect_address("localhost:50051"));

// 両方を持つノード
auto node = std::make_shared<rclcpp::Node>("my_node",
    rclcpp::NodeOptions()
        .set_server_address("0.0.0.0:50051")
        .set_connect_address("remote-host:50052"));
```

### rclcpp::Publisher\<T\>

メッセージを gRPC 経由で送信する Publisher。Stub を再利用するため効率的です。

```cpp
auto publisher = node->create_publisher<std_msgs::msg::Header>("topic_name", 10);

std_msgs::msg::Header header;
header.frame_id("my_frame");
header.stamp().sec(12345);
header.stamp().nanosec() = 6789;

publisher->publish(header);
```

### rclcpp::Subscription\<T\>

メッセージを受信し、コールバックで処理する Subscription。

```cpp
auto subscription = node->create_subscription<std_msgs::msg::Header>(
    "topic_name", 10,
    [](const std_msgs::msg::Header& header) {
        std::cout << "frame_id: " << header.frame_id() << std::endl;
    });

rclcpp::spin(node);
```

### rclcpp::Service\<SrvT\> / rclcpp::Client\<SrvT\>

ROS 2 スタイルのサービス（Request/Response パターン）。

```cpp
// サーバー側
auto service = node->create_service<example_interfaces::srv::AddTwoInts>(
    "add_two_ints",
    [](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> req,
       std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> res) {
        res->sum(static_cast<int64_t>(req->a()) + static_cast<int64_t>(req->b()));
    });

// クライアント側
auto client = node->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints");
auto request = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
request->a(1);
request->b(2);
auto future = client->async_send_request(request);
auto response = future.get();  // ブロック
```

### rclcpp::WallTimer

定期的にコールバックを実行するタイマー。

```cpp
using namespace std::chrono_literals;
auto timer = node->create_wall_timer(500ms, []() {
    RCLCPP_INFO(rclcpp::get_logger("timer"), "Timer fired!");
});
```

### ロギング

ROS 2 互換のロギングマクロ。

```cpp
RCLCPP_INFO(node->get_logger(), "Count: %d", count);
RCLCPP_WARN(node->get_logger(), "Warning: %s", message.c_str());
RCLCPP_ERROR(node->get_logger(), "Error code: %d", err);
RCLCPP_INFO_STREAM(node->get_logger(), "Value: " << value);
```

### ライフサイクル関数

```cpp
rclcpp::init(argc, argv);   // 初期化（SIGINT ハンドラも設定）
rclcpp::ok();               // 実行中かチェック
rclcpp::spin(node);         // ブロッキングスピン
rclcpp::spin_some(node);    // ノンブロッキングスピン
rclcpp::shutdown();         // シャットダウン
```

### メッセージアクセサ

生成されるラッパークラスは ROS 2 風のアクセサを提供します:

```cpp
// 文字列フィールド
header.frame_id("value");              // setter (関数呼び出し)
header.frame_id() = "value";           // setter (代入)
std::string id = header.frame_id();    // getter

// 数値フィールド
header.stamp().sec(100);               // setter (関数呼び出し)
header.stamp().sec() = 100;            // setter (代入)
int32_t s = header.stamp().sec();      // getter

// Repeated フィールド (配列)
tfmessage.transforms().resize(10);     // サイズ変更
tfmessage.transforms()[0] = value;     // 要素アクセス
size_t n = tfmessage.transforms().size(); // サイズ取得
```

### サンプルコード (ROS 2 スタイル)

**Publisher Node:**

```cpp
#include <chrono>
#include <rclcpp/rclcpp.hpp>
#include "tf2_msgs/msg/tfmessage.hpp"

using namespace std::chrono_literals;

class TFPublisher : public rclcpp::Node {
public:
    TFPublisher()
        : Node("tf_publisher",
               rclcpp::NodeOptions().set_connect_address("localhost:50051")),
          count_(0)
    {
        publisher_ = create_publisher<tf2_msgs::msg::TFMessage>("tf", 10);
        timer_ = create_wall_timer(
            100ms, std::bind(&TFPublisher::timer_callback, this));
    }

private:
    void timer_callback() {
        tf2_msgs::msg::TFMessage msg;
        msg.transforms().resize(1);
        msg.transforms()[0].header().frame_id("world");
        msg.transforms()[0].header().stamp().sec(count_++);
        msg.transforms()[0].child_frame_id() = "base_link";
        msg.transforms()[0].transform().translation().x(1.0);

        publisher_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published transform #%d", count_);
    }

    rclcpp::Publisher<tf2_msgs::msg::TFMessage>::SharedPtr publisher_;
    rclcpp::WallTimer::SharedPtr timer_;
    int count_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TFPublisher>());
    rclcpp::shutdown();
}
```

**Subscriber Node:**

```cpp
#include <rclcpp/rclcpp.hpp>
#include "tf2_msgs/msg/tfmessage.hpp"

class TFSubscriber : public rclcpp::Node {
public:
    TFSubscriber()
        : Node("tf_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50051"))
    {
        subscription_ = create_subscription<tf2_msgs::msg::TFMessage>(
            "tf", 10,
            [this](const tf2_msgs::msg::TFMessage& msg) {
                for (size_t i = 0; i < msg.transforms().size(); ++i) {
                    auto& t = msg.transforms()[i];
                    RCLCPP_INFO(get_logger(), "%s -> %s (%.1f, %.1f, %.1f)",
                        t.header().frame_id().c_str(),
                        t.child_frame_id().c_str(),
                        static_cast<double>(t.transform().translation().x()),
                        static_cast<double>(t.transform().translation().y()),
                        static_cast<double>(t.transform().translation().z()));
                }
            });
    }

private:
    rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr subscription_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TFSubscriber>());
    rclcpp::shutdown();
}
```

## サポートされているメッセージ型

以下の ROS 2 メッセージパッケージが変換可能です:

| パッケージ | 内容 |
| --- | --- |
| `std_msgs` | 基本型 (Bool, Int32, String, Header 等) |
| `builtin_interfaces` | Time, Duration |
| `geometry_msgs` | Point, Pose, Transform, Twist, Quaternion 等 |
| `sensor_msgs` | Image, PointCloud2, LaserScan, Imu, JointState 等 |
| `nav_msgs` | Odometry, Path, OccupancyGrid 等 |
| `tf2_msgs` | TFMessage |
| `visualization_msgs` | Marker, MarkerArray 等 |
| `diagnostic_msgs` | DiagnosticArray, DiagnosticStatus |
| `lifecycle_msgs` | State, Transition 等 |
| `stereo_msgs` | DisparityImage |
| `shape_msgs` | Mesh, Plane 等 |
| `trajectory_msgs` | JointTrajectory 等 |
| `example_interfaces` | AddTwoInts サービス等 |
| `std_srvs` | SetBool, Trigger, Empty サービス |

## 自作プロジェクトでの利用

### CMakeLists.txt の設定

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/opt/grpc" ${CMAKE_PREFIX_PATH})

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

include_directories(
    path/to/rclcpp/include   # rclcpp.hpp が置かれたディレクトリ
    /opt/grpc/include
    /opt/grpc-libs/include
    /opt/grpc-libs/include/std_msgs/msg
    /opt/grpc-libs/include/builtin_interfaces/msg
    /opt/grpc-libs/include/geometry_msgs/msg
    # 必要なメッセージ型のパスを追加
)

link_directories(/opt/grpc/lib /opt/grpc-libs/lib)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app
    gRPC::grpc++
    ${Protobuf_LIBRARIES}
    /opt/grpc-libs/lib/libstd_msgs.so
    /opt/grpc-libs/lib/libbuiltin_interfaces.so
    /opt/grpc-libs/lib/libgeometry_msgs.so
    # 必要なメッセージライブラリを追加
)
```

## ディレクトリ構造

```text
ros2-grpc-type-converter/
├── README.md
├── install_grpc.sh              # gRPC インストールスクリプト
├── install_library.sh           # 一括インストールスクリプト (Step 2〜4)
├── convert_idl_to_proto.py      # IDL → Proto 変換スクリプト
├── compile_install_ros_msgs.sh  # メッセージライブラリのコンパイル・インストール
├── make_access_header.py        # ROS 2 スタイルのラッパーヘッダー生成
├── cmake/
│   └── common.cmake             # CMake 共通設定
├── grpc_example/                # サンプルプロジェクト
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── rclcpp/
│   │       └── rclcpp.hpp       # ROS 2 互換 API ヘッダー
│   └── src/
│       ├── client.cpp           # Publisher サンプル (TFMessage)
│       ├── server.cpp           # Subscriber サンプル (TFMessage)
│       ├── cmd_vel_publisher.cpp    # Twist Publisher
│       ├── cmd_vel_subscriber.cpp   # Twist Subscriber
│       ├── image_publisher.cpp      # Image Publisher
│       ├── image_subscriber.cpp     # Image Subscriber
│       ├── imu_publisher.cpp        # Imu Publisher
│       ├── imu_subscriber.cpp       # Imu Subscriber
│       ├── joint_state_publisher.cpp  # JointState Publisher
│       ├── joint_state_subscriber.cpp # JointState Subscriber
│       ├── odom_publisher.cpp       # Odometry Publisher
│       ├── odom_subscriber.cpp      # Odometry Subscriber
│       ├── scan_publisher.cpp       # LaserScan Publisher
│       ├── scan_subscriber.cpp      # LaserScan Subscriber
│       ├── service_server.cpp       # AddTwoInts Service Server
│       ├── service_client.cpp       # AddTwoInts Service Client
│       ├── setbool_server.cpp       # SetBool Service Server
│       ├── setbool_client.cpp       # SetBool Service Client
│       └── test_all.cpp             # 統合テストスイート
└── ros-data-types-for-fastdds/  # ROS 2 IDL 定義 (Git サブモジュール)
    └── src/
        ├── std_msgs/
        ├── geometry_msgs/
        ├── sensor_msgs/
        └── ...
```

## トラブルシューティング

### ライブラリが見つからない

```text
error while loading shared libraries: libgrpc++.so: cannot open shared object file
```

**解決策**: `LD_LIBRARY_PATH` を設定してください。

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib:$LD_LIBRARY_PATH
```

### protoc がエラーを出す

```text
protoc: error: --proto_path: No such file or directory
```

**解決策**: Step 1 で gRPC が正しくインストールされたか確認してください。

```bash
ls /opt/grpc/bin/protoc
ls /opt/grpc/bin/grpc_cpp_plugin
```

### コンパイル時にヘッダーが見つからない

**解決策**: Step 4 のヘッダーコピーを実行済みか確認してください。

```bash
ls /opt/grpc-libs/include/std_msgs/msg/header.hpp
```

### サーバーに接続できない

- サーバーが先に起動しているか確認
- ファイアウォールでポート 50051 が開いているか確認
- リモート接続の場合、サーバー側は `0.0.0.0:50051` でリッスンしているか確認

## License

MIT License
