#!/bin/bash

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="${SCRIPT_DIR}/grpc_example/build"

export LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

echo "=== Building examples ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake ..
make -j$(nproc 2>/dev/null || echo 4)

echo ""
echo "=== Build complete ==="
echo "Binaries in: ${BUILD_DIR}"
echo ""
echo "Run examples:"
echo "  LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib ${BUILD_DIR}/subscriber_node"
echo "  LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib ${BUILD_DIR}/publisher_node"
echo ""
echo "Run tests:"
echo "  LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib ${BUILD_DIR}/test_all"
