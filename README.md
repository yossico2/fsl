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
        <local_port>5000</local_port>
        <remote_ip>127.0.0.1</remote_ip>
        <remote_port>6000</remote_port>
    </udp>
    <uds>
        <!-- app1 -->
        <server>/tmp/app1.fsl1.dl.high.sock</server>
        <server>/tmp/app1.fsl1.dl.low.sock</server>
        <client name="app1.ul">/tmp/app1.fsl1.ul.sock</client>
        <!-- app2 -->
        <server>/tmp/app2.fsl1.dl.high.sock</server>
        <server>/tmp/app2.fsl1.dl.low.sock</server>
        <client name="app2.ul">/tmp/app2.fsl1.ul.sock</client>
    </uds>
    <ul_uds_mapping>
        <mapping opcode="1" uds="app1.ul" />
        <mapping opcode="2" uds="app2.ul" />
    </ul_uds_mapping>
</config>
```

- `<udp>`: UDP socket configuration for FSL.
- `<uds>`: UDS server sockets (downlink) and client sockets (uplink) for each app.
- `<ul_uds_mapping>`: Maps message opcodes to UDS client names for routing uplink messages.

## Running the System

Build as described above, then run:

```bash
./build-debug/fsl
```

## Testing the System

### UDP → UDS client

Send a UDP packet to the FSL UDP port (e.g., 5000):

```bash
echo -n -e '<binary-packet>' | socat - UDP:127.0.0.1:5000
```
Or use Python to send a packet with a specific opcode and payload.

### UDS server → UDP

Send data to a UDS server socket:

```bash
echo -n "payload" | socat - UNIX-SENDTO:/tmp/app1.fsl1.dl.high.sock
```
FSL will send a UDP packet to the remote IP/port.

## Example Test Script

See `tests/test_config.cpp` for a config parser test using Catch2. To run all tests:

```bash
./build-debug/tests
```
