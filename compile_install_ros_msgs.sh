#!/bin/bash

set -e

# 使用法メッセージ
usage() {
    echo "Usage: $0 <base_directory>"
    exit 1
}

# 引数チェック
if [ $# -ne 1 ]; then
    usage
fi

BASE_DIR=$1

# ベースディレクトリの存在確認
if [ ! -d "$BASE_DIR" ]; then
    echo "Error: Directory $BASE_DIR does not exist."
    exit 1
fi

GRPC_DIR=/opt/grpc
INSTALL_DIR=/opt/grpc-libs

# ベースディレクトリ内のサブディレクトリを処理
for DIR in "$BASE_DIR"/*/; do
    # サブディレクトリがディレクトリであることを確認
    if [ -d "$DIR" ]; then
        # サブディレクトリ名を取得
        BASENAME=$(basename "$DIR")

        # .cc ファイルを再帰的に検索してコンパイル
        echo "Processing directory: $DIR"
        find "$DIR" -name "*.cc" > sources.txt

        if [ -s sources.txt ]; then
            # 出力ライブラリ名
            OUTPUT_LIB="lib${BASENAME}.so"

            # コンパイルコマンド
            g++ -shared -o "$OUTPUT_LIB" $(cat sources.txt) -fPIC \
                -I${GRPC_DIR}/include/ \
                -L${GRPC_DIR}/lib/ \
                -I$BASE_DIR/action_msgs/msg \
                -I$BASE_DIR/action_msgs/srv \
                -I$BASE_DIR/builtin_interfaces/msg \
                -I$BASE_DIR/diagnostic_msgs/msg \
                -I$BASE_DIR/diagnostic_msgs/srv \
                -I$BASE_DIR/example_interfaces/srv \
                -I$BASE_DIR/example_interfaces/action \
                -I$BASE_DIR/gazebo_msgs/msg \
                -I$BASE_DIR/gazebo_msgs/srv \
                -I$BASE_DIR/geometry_msgs/msg \
                -I$BASE_DIR/lifecycle_msgs/msg \
                -I$BASE_DIR/lifecycle_msgs/srv \
                -I$BASE_DIR/nav_msgs/msg \
                -I$BASE_DIR/nav_msgs/srv \
                -I$BASE_DIR/pendulum_msgs/msg \
                -I$BASE_DIR/rcl_interfaces/msg \
                -I$BASE_DIR/rcl_interfaces/srv \
                -I$BASE_DIR/sensor_msgs/msg \
                -I$BASE_DIR/sensor_msgs/srv \
                -I$BASE_DIR/shape_msgs/msg \
                -I$BASE_DIR/std_msgs/msg \
                -I$BASE_DIR/std_srvs/srv \
                -I$BASE_DIR/stereo_msgs/msg \
                -I$BASE_DIR/test_msgs/msg \
                -I$BASE_DIR/test_msgs/srv \
                -I$BASE_DIR/tf2_msgs/msg \
                -I$BASE_DIR/tf2_msgs/srv \
                -I$BASE_DIR/trajectory_msgs/msg \
                -I$BASE_DIR/unique_identifier_msgs/msg \
                -I$BASE_DIR/visualization_msgs/msg \
                -lprotobuf -lgrpc++ -lgrpc

            if [ $? -eq 0 ]; then
                echo "Successfully created $OUTPUT_LIB"
            else
                echo "Error: Failed to create $OUTPUT_LIB"
            fi

            # sources.txt を削除
            rm -f sources.txt
        else
            echo "No .cc files found in $DIR"
            rm -f sources.txt
        fi
    fi
done

sudo mkdir -p ${INSTALL_DIR}/lib
sudo cp -f lib*.so ${INSTALL_DIR}/lib/
sudo mkdir -p ${INSTALL_DIR}/include
sudo cp -rf $BASE_DIR/* ${INSTALL_DIR}/include/

# Install rclcpp headers (ROS 2 compatible API layer)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
if [ -d "${SCRIPT_DIR}/include" ]; then
    sudo cp -rf "${SCRIPT_DIR}/include/"* ${INSTALL_DIR}/include/
    echo "Installed rclcpp headers to ${INSTALL_DIR}/include/"
fi

rm -f ./lib*.so

echo "Installation complete: ${INSTALL_DIR}"
