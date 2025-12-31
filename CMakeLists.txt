# Catch2 test target
add_executable(tests
    tests/main.cpp
    tests/test_config.cpp
    tests/test_ctrl_status.cpp
    src/app.cpp
    src/sdk/logger.cpp
    src/config.cpp
    src/sdk/tinyxml2/tinyxml2.cpp
    src/sdk/udp.cpp
    src/sdk/uds.cpp
)
target_include_directories(tests PRIVATE tests src src/sdk/tinyxml2)
target_link_libraries(tests pthread)

cmake_minimum_required(VERSION 3.10)
project(fsl)

set(CMAKE_CXX_STANDARD 17)

# Set default build type to debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug CACHE STRING "Build type" FORCE)
endif()

# Platform selection
if(DEFINED ENV{PETALINUX})
    message(STATUS "Compiling for PetaLinux")
    # Example: set cross-compiler and flags for PetaLinux
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_C_COMPILER $ENV{PETALINUX_C_COMPILER})
    set(CMAKE_CXX_COMPILER $ENV{PETALINUX_CXX_COMPILER})
    add_definitions(-DPETALINUX)
else()
    message(STATUS "Compiling for Ubuntu/Linux")
endif()



add_executable(fsl src/main.cpp src/app.cpp src/sdk/logger.cpp src/config.cpp src/sdk/tinyxml2/tinyxml2.cpp src/sdk/udp.cpp src/sdk/uds.cpp)
target_include_directories(fsl PRIVATE src src/sdk/tinyxml2)
target_link_libraries(fsl pthread)

# Add include directories
include_directories(src)

# Copy config.xml to the build output directory after build
add_custom_command(TARGET fsl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/config.xml
        $<TARGET_FILE_DIR:fsl>/config.xml)
