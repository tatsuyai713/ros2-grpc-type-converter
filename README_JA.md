# ros2-grpc-type-converter

**言語:** [日本語（現在）](#) | [English](README.md)

---

## 🎯 何ができるか？

このライブラリは ROS 2 のメッセージ型定義 (IDL) を gRPC の Protocol Buffers (.proto) に自動変換し、**ROS 2 互換の API** で gRPC 通信を行えるようにします。ROS 2 をインストールせず、ROS 2 互換のメッセージ型を gRPC 経由で送受信できます。

### 主な機能

- **IDL ⟷ gRPC 変換**: ROS 2 メッセージ定義を Protocol Buffers に自動変換
- **ROS 2 互換 API**: `rclcpp::Publisher`、`rclcpp::Subscription`、`rclcpp::Service` など使用可
- **ROS 2 不要**: ROS 2 をインストールせず、スタンドアロン gRPC アプリケーションで実行
- **40+ メッセージ型**: std_msgs, geometry_msgs, sensor_msgs, nav_msgs など対応
- **サービス対応**: ROS 2 サービス完全対応（AddTwoInts、SetBool など）
- **9+ サンプル**: Pub/Sub、サービス、全メッセージ型の実装例
- **充実したテスト**: 56 個の自動統合テスト

---

## 💡 なぜ使うのか？

### 解決できる問題

1. **ROS 2 が使えない**: 組み込み、クラウド、エッジデバイスでのデプロイ
2. **クロスプラットフォーム**: 標準 gRPC と ROS 2 メッセージを組み合わせて通信
3. **軽量**: 単一バイナリ、ROS 2 の重いミドルウェア不要
4. **簡単な統合**: ROS 2 API で実装できるため、移行が容易
5. **デバッグが簡単**: Protocol Buffers は言語非依存で検査しやすい

### 活用例

- ロボットからのセンサデータ収集
- クラウドでのロボット遠隔操作
- ROS 2 との異なるシステム統合
- 完全な ROS 2 環境がない開発環境

---

## 🚀 クイックスタート

### インストール（2ステップ）

```bash
# 1. gRPC をビルド・インストール
./install_grpc.sh

# 2. ROS 2 メッセージライブラリをビルド・インストール（ステップ 2-4 を一括実行）
./install_library.sh
```

### サンプルのビルド

```bash
cd grpc_example
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### サンプルの実行

**Publisher（クライアント側）:**
```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./publisher_node
```

**Subscriber（サーバー側）:**
```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./subscriber_node
```

### テスト実行

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./test_all  # 56 個の自動統合テスト
```

---

## 📖 使い方

### 基本的な Publisher（クライアント側）

```cpp
#include <rclcpp/rclcpp.hpp>
#include "geometry_msgs/msg/twist.hpp"

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rclcpp::Node>(
        "cmd_vel_publisher",
        rclcpp::NodeOptions().set_connect_address("localhost:50053"));

    auto publisher = node->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

    geometry_msgs::msg::Twist msg;
    msg.linear().x(1.0);
    msg.angular().z(0.5);

    publisher->publish(msg);

    rclcpp::shutdown();
}
```

### 基本的な Subscriber（サーバー側）

```cpp
#include <rclcpp/rclcpp.hpp>
#include "geometry_msgs/msg/twist.hpp"

class CmdVelSubscriber : public rclcpp::Node {
public:
    CmdVelSubscriber()
        : Node("cmd_vel_subscriber",
               rclcpp::NodeOptions().set_server_address("0.0.0.0:50053"))
    {
        subscription_ = create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10,
            [](const geometry_msgs::msg::Twist& msg) {
                std::cout << "Linear: " << msg.linear().x()
                          << ", Angular: " << msg.angular().z() << std::endl;
            });
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelSubscriber>());
    rclcpp::shutdown();
}
```

### サービスサーバー

```cpp
auto service = node->create_service<example_interfaces::srv::AddTwoInts>(
    "add_two_ints",
    [](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> req,
       std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> res) {
        res->sum(req->a() + req->b());
    });
```

### サービスクライアント

```cpp
auto client = node->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints");
auto request = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
request->a(3);
request->b(5);
auto future = client->async_send_request(request);
auto response = future.get();
std::cout << "Sum: " << response->sum() << std::endl;  // 出力: 8
```

---

## 📚 利用可能なサンプル

| サンプル | メッセージ型 | ポート | 説明 |
| --- | --- | --- | --- |
| `publisher_node` / `subscriber_node` | `tf2_msgs/TFMessage` | 50051 | TF の Pub/Sub |
| `cmd_vel_publisher` / `cmd_vel_subscriber` | `geometry_msgs/Twist` | 50053 | 速度指令 |
| `image_publisher` / `image_subscriber` | `sensor_msgs/Image` | 50054 | 画像データ |
| `imu_publisher` / `imu_subscriber` | `sensor_msgs/Imu` | 50055 | IMU センサデータ |
| `joint_state_publisher` / `joint_state_subscriber` | `sensor_msgs/JointState` | 50056 | 関節角度 |
| `odom_publisher` / `odom_subscriber` | `nav_msgs/Odometry` | 50057 | オドメトリ |
| `scan_publisher` / `scan_subscriber` | `sensor_msgs/LaserScan` | 50058 | レーザースキャン |
| `service_server` / `service_client` | `example_interfaces/AddTwoInts` | 50052 | 整数加算 |
| `setbool_server` / `setbool_client` | `std_srvs/SetBool` | 50059 | Bool サービス |

---

## 🏗️ アーキテクチャ

```text
IDL ファイル (.idl)
      ↓ (convert_idl_to_proto.py)
Proto ファイル (.proto)
      ↓ (protoc)
Protocol Buffer C++ コード (.pb.h, .pb.cc)
      ↓ (make_access_header.py)
ROS 2 互換ラッパーヘッダー (.hpp)
      ↓ (compile_install_ros_msgs.sh)
共有ライブラリ (.so)
      ↓
あなたの gRPC アプリケーション
```

### 通信モデル

- **Publisher**（クライアント側）: gRPC ユナリ RPC でメッセージ送信（Stub 再利用で効率的）
- **Subscription**（サーバー側）: gRPC サービスを実装、受信メッセージをコールバックで処理
- **Service**（サーバー側）: Request/Response パターンの gRPC 操作を実装
- **Client**（クライアント側）: サービスへの非同期リクエスト送信
- **Timer**: WallTimer でコールバックを定期実行

---

## 🔧 コンポーネント

| ファイル | 役割 |
| --- | --- |
| `convert_idl_to_proto.py` | IDL ファイルを Proto ファイルに変換し、protoc でコンパイル |
| `make_access_header.py` | protoc 生成の `.pb.h` を解析し、ROS 2 スタイルの C++ ラッパーヘッダーを生成 |
| `compile_install_ros_msgs.sh` | 変換 C++ ソースを共有ライブラリにコンパイルしインストール |
| `install_grpc.sh` | gRPC v1.66.1 をソースからビルドしインストール |
| `install_library.sh` | ステップ 2-4 を一括実行するスクリプト |
| `grpc_example/include/rclcpp/rclcpp.hpp` | **ROS 2 互換 API レイヤー** — Publisher, Subscription, Service, Client, Timer, Node |
| `grpc_example/src/` | すべてのメッセージ型対応のサンプルプログラム |

---

## 📋 対応メッセージ型

| パッケージ | 内容 |
| --- | --- |
| `std_msgs` | Bool, Int32, String, Header など |
| `builtin_interfaces` | Time, Duration |
| `geometry_msgs` | Point, Pose, Transform, Twist, Quaternion など |
| `sensor_msgs` | Image, PointCloud2, LaserScan, Imu, JointState など |
| `nav_msgs` | Odometry, Path, OccupancyGrid など |
| `tf2_msgs` | TFMessage |
| `visualization_msgs` | Marker, MarkerArray など |
| `diagnostic_msgs` | DiagnosticArray, DiagnosticStatus |
| `lifecycle_msgs` | State, Transition など |
| `stereo_msgs` | DisparityImage |
| `shape_msgs` | Mesh, Plane など |
| `trajectory_msgs` | JointTrajectory など |
| `example_interfaces` | AddTwoInts サービスなど |
| `std_srvs` | SetBool, Trigger, Empty サービス |

---

## 📋 API リファレンス

### rclcpp::Node

```cpp
auto node = std::make_shared<rclcpp::Node>(
    "node_name",
    rclcpp::NodeOptions()
        .set_server_address("0.0.0.0:50051")      // Subscription・Service 用
        .set_connect_address("localhost:50051"));  // Publisher・Client 用
```

### rclcpp::Publisher\<T\>

```cpp
auto publisher = node->create_publisher<std_msgs::msg::String>("topic", 10);
std_msgs::msg::String msg;
msg.data("Hello");
publisher->publish(msg);
```

### rclcpp::Subscription\<T\>

```cpp
auto subscription = node->create_subscription<std_msgs::msg::String>(
    "topic", 10,
    [](const std_msgs::msg::String& msg) {
        std::cout << msg.data() << std::endl;
    });
```

### rclcpp::WallTimer

```cpp
using namespace std::chrono_literals;
auto timer = node->create_wall_timer(
    100ms,
    []() { std::cout << "Timer fired!" << std::endl; });
```

### ロギング

```cpp
auto logger = rclcpp::get_logger("my_logger");
RCLCPP_INFO(logger, "Info: %d", 42);
RCLCPP_WARN(logger, "Warning: %s", "注意");
RCLCPP_ERROR(logger, "Error: %.2f", 3.14);
RCLCPP_INFO_STREAM(logger, "Stream: " << value);
```

---

## ⚙️ インストール詳細

### 前提条件

- **OS**: Ubuntu 20.04, 22.04, 24.04（x86_64）
- **コンパイラ**: GCC 9+（C++17 対応）
- **CMake**: 3.10+
- **Python**: 3.8+
- **ディスク**: 約10GB（gRPC ビルド用）
- **権限**: `/opt/grpc` と `/opt/grpc-libs` への sudo アクセス

### ステップ別インストール

#### ステップ 1: リポジトリのクローン

```bash
git clone --recursive https://github.com/tatsuyai713/lwrcl-grpc.git
cd lwrcl-grpc
```

`--recursive` なしでクローンした場合:

```bash
git submodule update --init --recursive
```

#### ステップ 2: gRPC インストール

```bash
./install_grpc.sh  # 30分〜2時間
```

**インストール先**: `/opt/grpc`

#### ステップ 3: ワンショットライブラリインストール

```bash
./install_library.sh
```

以下をまとめて実行：
- IDL → Proto 変換
- ROS 2 ライブラリのコンパイル
- ラッパーヘッダーの生成

**インストール先**: `/opt/grpc-libs`

#### ステップ 4: サンプルのビルド

```bash
cd grpc_example
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

---

## 🐛 トラブルシューティング

### ライブラリが見つからない

```
error while loading shared libraries: libgrpc++.so
```

環境変数を設定：

```bash
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib:$LD_LIBRARY_PATH
```

### ヘッダーが見つからない

ヘッダーがインストールされているか確認：

```bash
ls /opt/grpc-libs/include/std_msgs/msg/header.hpp
```

### protoc バージョンの不一致

**解決策**: gRPC ステップ 1 が正常に完了しているか確認：

```bash
/opt/grpc/bin/protoc --version
/opt/grpc/bin/grpc_cpp_plugin --version
```

### サーバーに接続できない

- サーバーが先に起動しているか確認
- ファイアウォールでポート（例: 50051）が開いているか確認
- リモート接続の場合、サーバーは `0.0.0.0:PORT` でリスンしているか確認

---

## 🧪 テスト

統合テストスイート（56 個のテスト）を実行：

```bash
cd grpc_example/build
export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib
./test_all
```

期待される出力：
```
========================================
  ros2-grpc-type-converter Test Suite
========================================
...
========================================
  Results: 56 passed, 0 failed
========================================
```

---

## 📝 制限事項

- **一方向**: Publisher → Subscription はクライアント → サーバーのみ
- **バイト配列**: `byte` と `octet` 型は `uint32` 配列に変換（メモリ 4 倍）
- **QoS**: ROS 2 QoS ポリシーは未実装（API 互換性のため引数は受け入れ）
- **ROS 2 共存**: ライブラリ名が競合; ROS 2 との同時実行不可
- **ディスカバリー**: 自動 DDS ディスカバリーなし; 明示的なアドレス指定が必要
- **同一型制限**: ノードあたり最大 1 つの Subscription/Service

---

## 📄 ライセンス

MIT License

---

## 🔗 関連リンク

- [ROS 2 Documentation](https://docs.ros.org/en/humble/)
- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)

---

**最終更新**: 2026-03-28
**gRPC バージョン**: v1.66.1
**ステータス**: ✅ 積極的にメンテナンス中
