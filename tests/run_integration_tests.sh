#!/bin/bash
# Run FSL integration tests end-to-end
# Usage: ./run_integration_tests.sh

set -e

# Parse build type and target from args or environment
BUILD_TYPE="${BUILD_TYPE:-debug}"
TARGET="${TARGET:-linux}"
while [[ $# -gt 0 ]]; do
	key="$1"
	case $key in
	-b | --build)
		if [[ "$2" == "release" || "$2" == "release" ]]; then
			BUILD_TYPE="release"
		elif [[ "$2" == "debug" || "$2" == "debug" ]]; then
			BUILD_TYPE="debug"
		fi
		shift
		shift
		;;
	-t | --target)
		if [[ "$2" == "linux" || "$2" == "Linux" ]]; then
			TARGET="linux"
		elif [[ "$2" == "petalinux" || "$2" == "PetaLinux" || "$2" == "petalinux" ]]; then
			TARGET="petalinux"
		fi
		shift
		shift
		;;
	*) shift ;;
	esac
done

BUILD_DIR="build"
BUILD_TARGET_DIR="${BUILD_DIR}/${TARGET}/$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${BUILD_TARGET_DIR}"

# Ensure logs directory exists under project root
LOGDIR="../logs"
mkdir -p "${LOGDIR}"

# Remove previous log file if it exists
if [ -f "${LOGDIR}/fsl-${STATEFULSET_INDEX}.log" ]; then
	rm -f "${LOGDIR}/fsl-${STATEFULSET_INDEX}.log"
fi

# Start FSL in background, redirect output to logs/fsl-${STATEFULSET_INDEX}.log
./fsl >"${LOGDIR}/fsl-${STATEFULSET_INDEX}.log" 2>&1 &
FSL_PID=$!

# Wait for FSL to initialize
sleep 1

# Change to project root before running integration tests
cd "${PROJECT_ROOT}"
python3 tests/integration_tests.py
TEST_RESULT=$?

# Give FSL time to process and log any final requests
sleep 1

# Kill FSL
kill ${FSL_PID}
wait ${FSL_PID} 2>/dev/null || true

exit ${TEST_RESULT}
