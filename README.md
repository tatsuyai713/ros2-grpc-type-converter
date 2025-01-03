# ros2-grpc-type-converter

## Limitations

- Only support client to server communication
- byte array is converted to uint32_t array (need 4 times memory size)

## Clone repository

```shell
git clone --recursive https://github.com/example/ros2-grpc-type-converter.git
```

## Usage

### Install gRPC (C++)

```shell
./install_grpc.sh
```

Install path : /opt/grpc

### Convert IDL to proto

```shell
LD_LIBRARY_PATH=/opt/grpc/lib python3 ./convert_idl_to_proto.py ./ros-data-types-for-fastdds/src/
```

### Compile proto and install ROS 2 type message libraries

```shell
LD_LIBRARY_PATH=/opt/grpc/lib ./compile_install_ros_msgs.sh ./ros-data-types-for-fastdds/src/
```

Install path : /opt/grpc-libs

### Make access headers for ROS 2 style data access

```shell
python3 ./make_access_header.py ./ros-data-types-for-fastdds/src/
sudo cp -rf ./ros-data-types-for-fastdds/src/* /opt/grpc-libs/include/
```

### Build examples

```shell
cd grpc_example
mkdir build
cd build
cmake ..
make
```

### Execute sample

#### Execute server

```shell
LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib ./server
```

#### Execute client

```shell
LD_LIBRARY_PATH=/opt/grpc/lib:/opt/grpc-libs/lib ./client
```

## License

MIT License
