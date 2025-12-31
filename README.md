# fsl C++ Application

## Quick Start

```bash
./make.sh
./build/linux/debug/fsl
```

## Example config.xml

See `src/config.xml` for a full example. Key sections:

```xml
<config>
    <logging>
        <level>INFO</level>
    </logging>
    <sensor_id>0</sensor_id>
    <udp>
        <local_port>9910</local_port>
        <remote_ip>127.0.0.1</remote_ip>
        <remote_port>9010</remote_port>
    </udp>
    <data_link_uds>
        <server name="DL_EL_H">
            <path>/tmp/DL_EL_H</path>
            <receive_buffer_size>65500</receive_buffer_size>
        </server>
        <!-- ... more servers/clients ... -->
    </data_link_uds>
    <ul_uds_mapping>
        <mapping opcode="1" uds="FSW_UL" />
    </ul_uds_mapping>
    <ctrl_status_uds>
        <FSW>
            <request>
                <path>/tmp/fsw_to_fcom</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </request>
            <response>
                <path>/tmp/fcom_to_fsw</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </response>
        </FSW>
        <!-- ... more app sections ... -->
    </ctrl_status_uds>
</config>
```

## Building for Ubuntu/Linux

### Building with make.sh

You can use the provided script to build for different configurations and targets:

```bash
# Default (debug, Linux)
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

# FSL: Flexible Socket Layer (C++)

FSL is a C++ application providing a flexible socket layer for message routing between UDP and Unix Domain Sockets (UDS). It is designed for both Ubuntu/Linux and PetaLinux environments, with a focus on modular configuration and robust testing.

Replace `<path-to-petalinux-gcc>` and `<path-to-petalinux-g++>` with the actual paths to your PetaLinux toolchain binaries.

## Source Structure
- `src/main.cpp`: Entry point
- `src/example.cpp`, `src/example.h`: Example function with platform-specific output

## Configuration (`config.xml`)

## Building (Ubuntu/Linux)


The `make.sh` script supports several commands for building, testing, cleaning, and Docker image creation:

```bash
# Show usage
./make.sh -h

# Clean build directories
./make.sh clean

# Build (default: debug, Linux)
./make.sh

# Run all: clean, build, and test
./make.sh all

# Run unit and integration tests
./make.sh test

# Build Docker image (tag: fsl)
./make.sh image

# Specify build type (debug or release)
./make.sh -b debug
./make.sh -b release

# Specify target (linux or petalinux)
./make.sh -t linux
./make.sh -t petalinux

# Combine options
./make.sh -b release -t petalinux
```

The script uses separate build directories for each configuration.

```bash
# Default (debug, Linux)
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

The script uses separate build directories for each configuration.

### Cleaning build files

To remove all build files, run:

```bash
cd build
make clean
```

No cleanup script is needed. The `build` directory is ignored by git via `.gitignore`.
                <path>/tmp/fsw_to_fcom</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </request>
            <response>
                <path>/tmp/fcom_to_fsw</path>
                <receive_buffer_size>1024</receive_buffer_size>
            </response>
        </fsw>
        <PLMG>

# Then build:
mkdir -p build
cd build
cmake ..
make
```

Replace `<path-to-petalinux-gcc>` and `<path-to-petalinux-g++>` with the actual paths to your PetaLinux toolchain binaries.
            </response>
        </PLMG>

- `src/main.cpp`: Application entry point
- `src/app.cpp`, `src/app.h`: Main application logic
- `src/config.cpp`, `src/config.h`: XML configuration parser
- `src/sdk/logger.cpp`, `src/logger.h`: Logging utilities
- `src/sdk/udp.cpp`, `src/sdk/udp.h`: UDP socket handling
- `src/sdk/uds.cpp`, `src/sdk/uds.h`: Unix Domain Socket handling
- `src/config.xml`: configuration file
```

- `<udp>`: UDP socket configuration for FSL.
- `<data_link_uds>`: UDS server sockets (downlink) and client sockets (uplink) for each app.
- `<ul_uds_mapping>`: Maps message opcodes to UDS client names for routing uplink messages.
- `<ctrl_status_uds>`: Contains ctrl/status UDS channels for each app or logical entity. Each child element (e.g., `<FSW>`, `<telemetry>`, `<PLMG>`, `<EL>`, `<dynamic>`) defines request and/or response UDS sockets for control and status communication between the app and FSL. Each `<request>` or `<response>` can specify a `<path>` and an optional `<receive_buffer_size>`. This section is parsed dynamically, so you can add or remove app sections as needed.


## Running the System

Build as described above, then run:

```bash
./build/linux/debug/fsl
```

## Testing

### C++ Unit Tests

Unit tests use Catch2. To run all C++ tests:

```bash
./build/linux/debug/tests
```

See `tests/test_config.cpp` for a config parser test.

### Integration Tests (Python)

Integration tests and scripts are in the `tests/` directory:

- `integration_tests.py`: Python integration test
- `run_integration_tests.sh`: Script to run integration tests

See `tests/integration_tests.md` for details.

### Manual Testing

#### UDP → UDS client

Send a UDP packet to the FSL UDP port (e.g., 9910):

```bash
echo -n -e '<binary-packet>' | socat - UDP:127.0.0.1:9910
```
Or use Python to send a packet with a specific opcode and payload.

#### UDS server → UDP

Send data to a UDS server socket:

```bash
echo -n "payload" | socat - UNIX-SENDTO:/tmp/FSW_HIGH_DL
```
FSL will send a UDP packet to the remote IP/port.

## Troubleshooting

- If you see "Failed to load XML file", check that `config.xml` exists and is valid.
- For socket errors, ensure no other process is using the same UDS/UDP ports.
- For multi-instance, verify unique UDS paths and UDP ports per instance.

## Environment Variable Override

You can override the UDP configuration from `config.xml` by setting the following environment variables before running FSL:

- `FSL_LOCAL_PORT`: Overrides the UDP local port
- `FSL_REMOTE_IP`:  Overrides the UDP remote IP address
- `FSL_REMOTE_PORT`: Overrides the UDP remote port

Example usage:

```bash
FSL_LOCAL_PORT=1234 FSL_REMOTE_IP=1.2.3.4 FSL_REMOTE_PORT=5678 ./linux/build/debug/fsl
```

If set, these variables take precedence over the values in `config.xml`.

## License

Add your license information here (e.g., MIT, Apache 2.0, etc.).
