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

## Notes
- The build system auto-detects PetaLinux via the `PETALINUX` environment variable.
- For Ubuntu/Linux, no special environment setup is required.
