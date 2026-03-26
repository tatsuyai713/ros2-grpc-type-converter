#!/bin/bash

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
SRC_DIR="${SCRIPT_DIR}/ros-data-types-for-fastdds/src"
GRPC_DIR=/opt/grpc

export LD_LIBRARY_PATH=${GRPC_DIR}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

echo "=== Step 1/3: Converting IDL to Proto ==="
python3 "${SCRIPT_DIR}/convert_idl_to_proto.py" "${SRC_DIR}"

echo ""
echo "=== Step 2/3: Compiling ROS message libraries ==="
bash "${SCRIPT_DIR}/compile_install_ros_msgs.sh" "${SRC_DIR}"

echo ""
echo "=== Step 3/3: Generating access headers ==="
python3 "${SCRIPT_DIR}/make_access_header.py" "${SRC_DIR}"

echo ""
echo "=== Installation complete ==="
echo "Libraries: /opt/grpc-libs/lib"
echo "Headers:   /opt/grpc-libs/include"
echo ""
echo "Add to your environment:"
echo "  export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib:\$LD_LIBRARY_PATH"
