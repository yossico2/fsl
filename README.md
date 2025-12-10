# fsl C++ Application

This project is a simple C++ application using CMake, designed to compile for either Ubuntu/Linux or PetaLinux.


## Build requirements
Install tinyxml2:
```bash
sudo apt-get install libtinyxml2-dev
```

## Building for Ubuntu/Linux


### Building with make.sh

You can use the provided script to build for different configurations and targets:

```bash
# Default (Debug, Linux)
./make.sh

# Specify build type (debug or release)
./make.sh -b debug
./make.sh -b release

# Specify target (linux or petalinux)
./make.sh -t linux
./make.sh -t petalinux

# Combine options
./make.sh -b release -t petalinux
```

The script will automatically use separate build directories for each configuration.

## Cleaning build files

To remove all build files, run:

```bash
make clean
```

Run this command inside the `build` directory after building with CMake. No cleanup.sh script is needed.

## .gitignore

The build directory is ignored by git using the provided `.gitignore` file.
./fsl
```

## Building for PetaLinux

Set the following environment variables to point to your PetaLinux cross-compilers before running CMake:

```bash
export PETALINUX=1
export PETALINUX_C_COMPILER=<path-to-petalinux-gcc>
export PETALINUX_CXX_COMPILER=<path-to-petalinux-g++>
mkdir -p build
cd build
cmake ..
make
```

Replace `<path-to-petalinux-gcc>` and `<path-to-petalinux-g++>` with the actual paths to your PetaLinux toolchain binaries.

## Source Structure
- `src/main.cpp`: Entry point
- `src/example.cpp`, `src/example.h`: Example function with platform-specific output

## Configuration (`config.xml`)

The system is configured via an XML file. Example:

```xml
<config>
    <udp>
        <local_port>9910</local_port>
        <remote_ip>127.0.0.1</remote_ip>
        <remote_port>9010</remote_port>
    </udp>
    <data_link_uds>
        <!-- app1 -->
        <server>
            <path>/tmp/DL_APP1_H</path>
            <receive_buffer_size>1024</receive_buffer_size>
        </server>
        <server>
            <path>/tmp/DL_APP1_L</path>
            <receive_buffer_size>1024</receive_buffer_size>
        </server>
        <client name="app1.ul">UL_APP1</client>
        <!-- app2 -->
        <server>
            <path>/tmp/DL_APP2_H</path>
            <receive_buffer_size>1024</receive_buffer_size>
        </server>
        <server>
            <path>/tmp/DL_APP2_L</path>
            <receive_buffer_size>1024</receive_buffer_size>
        </server>
        <client name="app2.ul">UL_APP2</client>
    </data_link_uds>
    <ul_uds_mapping>
        <mapping opcode="1" uds="app1.ul" />
        <mapping opcode="2" uds="app2.ul" />
    </ul_uds_mapping>
    <ctrl_status_uds>
        <app1>
            <request>
                <path>/tmp/app1_to_fcom</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </request>
            <response>
                <path>/tmp/fcom_to_app1</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </response>
        </app1>
        <app2>
            <request>
                <path>/tmp/app2_to_fcom</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </request>
            <response>
                <path>/tmp/fcom_to_app2</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </response>
        </app2>
        <tod>
            <request>
                <path>/tmp/app2_to_fcom</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </request>
        </tod>
    </ctrl_status_uds>
</config>
```

- `<udp>`: UDP socket configuration for FSL.
- `<data_link_uds>`: UDS server sockets (downlink) and client sockets (uplink) for each app.
- `<ul_uds_mapping>`: Maps message opcodes to UDS client names for routing uplink messages.
- `<ctrl_status_uds>`: Contains ctrl/status UDS channels for each app or logical entity. Each child element (e.g., `<app1>`, `<app2>`, `<tod>`) defines request and/or response UDS sockets for control and status communication between the app and FSL. Each `<request>` or `<response>` can specify a `<path>` and an optional `<receive_buffer_size>`. This section is parsed dynamically, so you can add or remove app sections as needed.

## Running the System

Build as described above, then run:

```bash
./build-debug/fsl
```

## Testing the System

### UDP → UDS client

Send a UDP packet to the FSL UDP port (e.g., 9910):

```bash
echo -n -e '<binary-packet>' | socat - UDP:127.0.0.1:9910
```
Or use Python to send a packet with a specific opcode and payload.

### UDS server → UDP

Send data to a UDS server socket:

```bash
echo -n "payload" | socat - UNIX-SENDTO:/tmp/DL_APP1_H
```
FSL will send a UDP packet to the remote IP/port.

## Example Test Script

See `tests/test_config.cpp` for a config parser test using Catch2. To run all tests:

```bash
./build-debug/tests
```

## Environment Variable Override

You can override the UDP configuration from `config.xml` by setting the following environment variables before running FSL:

- `FSL_LOCAL_PORT`: Overrides the UDP local port
- `FSL_REMOTE_IP`:  Overrides the UDP remote IP address
- `FSL_REMOTE_PORT`: Overrides the UDP remote port

Example usage:

```bash
FSL_LOCAL_PORT=1234 FSL_REMOTE_IP=1.2.3.4 FSL_REMOTE_PORT=5678 ./build-debug/fsl
```

If set, these variables take precedence over the values in `config.xml`.
