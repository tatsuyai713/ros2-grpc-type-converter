#!/bin/bash

set -e

VERSION=v1.66.1

GRPC_SOURCE_DIR=${PWD}/grpc

CMAKE_DIR=${GRPC_SOURCE_DIR}/cmake
GRPC_BUILD_DIR=${CMAKE_DIR}/build

INSTALL_DIR=/opt/grpc

BUILD_TYPE=Release
BUILD_PARALLEL=$(nproc 2>/dev/null || echo 4)

echo "=== Installing gRPC ${VERSION} ==="
echo "Install directory: ${INSTALL_DIR}"
echo "Build parallelism: ${BUILD_PARALLEL}"

# Install dependencies
sudo apt-get update
sudo apt-get install -y ca-certificates gpg wget build-essential autoconf libtool pkg-config libsystemd-dev

# Install latest cmake 3.x from Kitware repository
test -f /usr/share/doc/kitware-archive-keyring/copyright ||
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null

sudo apt-get update

# Find and install the latest cmake 3.x version (pin to 3.x to avoid cmake 4.x breakage)
CMAKE_CURRENT=$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')
if [[ "${CMAKE_CURRENT}" == 3.* ]]; then
    echo "cmake ${CMAKE_CURRENT} (3.x) is already installed, keeping it"
    # Hold cmake packages to prevent accidental upgrade to 4.x
    sudo apt-mark hold cmake cmake-data 2>/dev/null || true
else
    CMAKE_LATEST_3=$(apt-cache madison cmake | awk -F'|' '{print $2}' | tr -d ' ' | grep '^3\.' | sort -V | tail -1)
    if [ -n "${CMAKE_LATEST_3}" ]; then
        echo "Installing cmake ${CMAKE_LATEST_3} (latest 3.x)"
        sudo apt-get install -y --allow-downgrades cmake="${CMAKE_LATEST_3}" cmake-data="${CMAKE_LATEST_3}"
        sudo apt-mark hold cmake cmake-data
    else
        echo "Error: No cmake 3.x found in repository"
        exit 1
    fi
fi
echo "Using cmake: $(cmake --version | head -1)"

# Clone gRPC
rm -rf ${GRPC_SOURCE_DIR}
mkdir -p ${GRPC_SOURCE_DIR}
pushd ${GRPC_SOURCE_DIR}
git clone --recursive -b ${VERSION} https://github.com/grpc/grpc .
git submodule update --init --recursive
popd

# Build gRPC
sudo rm -rf ${INSTALL_DIR}

rm -rf ${GRPC_BUILD_DIR}
mkdir -p ${GRPC_BUILD_DIR}
pushd ${GRPC_BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DgRPC_BUILD_TESTS=OFF \
      -DgRPC_BUILD_CSHARP=OFF \
      -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
      -DgRPC_BUILD_GRPC_CPP_PLUGIN=ON \
      -DgRPC_BUILD_SHARED_LIBS=ON \
      -DgRPC_INSTALL=ON \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
      -DBUILD_SHARED_LIBS=ON \
      -Dprotobuf_BUILD_SHARED_LIBS=ON \
      -DCMAKE_PREFIX_PATH=${INSTALL_DIR} \
      ${GRPC_SOURCE_DIR}

make -j${BUILD_PARALLEL}

sudo mkdir -p ${INSTALL_DIR}
sudo make install
popd

echo "=== gRPC ${VERSION} installed to ${INSTALL_DIR} ==="
echo ""
echo "Add to your environment:"
echo "  export LD_LIBRARY_PATH=${INSTALL_DIR}/lib:\$LD_LIBRARY_PATH"
