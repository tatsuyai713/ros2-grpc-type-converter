# ros2-grpc-type-converter

## Limitations

- Only support client to server communication
- "header().stamp.sec() = 100;" style is not supported
- byte array convert to uint32_t array (need 4 times memory size)

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
python3 ./convert_idl_to_proto.py
```

### Compile proto and install ROS 2 type message libraries

```shell
./compile_install_ros_msgs.sh
```

Install path : /opt/grpc-libs

### Make access headers for ROS 2 style data access

```shell
python3 ./make_access_header.py
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
