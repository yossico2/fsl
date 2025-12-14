#!/bin/bash
# env.sh - Centralized environment variables for build scripts

# Only set defaults if not already set
if [[ -z "${BUILD_TYPE}" ]]; then
	BUILD_TYPE="debug"
fi
if [[ -z "${BUILD_DIR}" ]]; then
	BUILD_DIR="build"
fi
if [[ -z "${TARGET}" ]]; then
	TARGET="linux"
fi
# Always set BUILD_TARGET_DIR to build/<target>/<build_type> (lowercase)
BUILD_TARGET_DIR="${BUILD_DIR}/${TARGET}/$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"

export BUILD_TYPE
export BUILD_DIR
export TARGET
export BUILD_TARGET_DIR
