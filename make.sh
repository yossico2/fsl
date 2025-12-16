#!/bin/bash

set -e

function print_usage() {
	echo "Usage: $0 [clean|test|all|image] [-b|--build debug|release] [-t|--target linux|petalinux] [-h|--help]"
	echo "  clean         Remove build folder"
	echo "  test          Run unit and integration tests"
	echo "  all           Run clean, build, and test"
	echo "  image         Build the Docker image (fsl-app)"
	echo "  -b, --build   Build type: debug (default) or release"
	echo "  -t, --target  Target platform: linux (default) or petalinux"
	echo "  -h, --help    Show this help message"
}

function main() {
	BUILD_TYPE="debug"
	BUILD_DIR="build"
	TARGET="linux"

	# Parse all arguments, allowing options before or after the positional argument
	POS_ARG=""
	REMAINING_ARGS=()
	while [[ $# -gt 0 ]]; do
		key="${1}"
		case ${key} in
		-b | --build)
			if [[ -n "${2}" && ("${2}" == "release" || "${2}" == "Release") ]]; then
				BUILD_TYPE="release"
			elif [[ -n "${2}" && ("${2}" == "debug" || "${2}" == "Debug") ]]; then
				BUILD_TYPE="debug"
			else
				echo "Unknown build type: ${2}. Use 'debug' or 'release'."
				exit 1
			fi
			shift
			shift
			;;
		-t | --target)
			if [[ -n "${2}" && ("${2}" == "linux" || "${2}" == "Linux") ]]; then
				TARGET="linux"
			elif [[ -n "${2}" && ("${2}" == "petalinux" || "${2}" == "PetaLinux") ]]; then
				TARGET="petalinux"
			else
				echo "Unknown target: ${2}. Use 'linux' or 'petalinux'."
				exit 1
			fi
			shift
			shift
			;;
		-h | --help)
			print_usage
			exit 0
			;;
		clean | test | all | image)
			if [[ -z "$POS_ARG" ]]; then
				POS_ARG="$1"
			else
				REMAINING_ARGS+=("$1")
			fi
			shift
			;;
		*)
			REMAINING_ARGS+=("$1")
			shift
			;;
		esac
	done

	# Export variables for env.sh and external use
	export BUILD_TYPE
	export BUILD_DIR
	export TARGET

	# Compute BUILD_TARGET_DIR as build/<target>/<build_type>
	BUILD_TARGET_DIR="${BUILD_DIR}/${TARGET}/$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
	export BUILD_TARGET_DIR

	case "${POS_ARG}" in
	clean)
		echo -e "\e[97;44mCleaning build folder: ${BUILD_TARGET_DIR}...\e[0m"
		rm -rf "${BUILD_TARGET_DIR}"
		echo -e "\e[32mClean completed.\e[0m"
		exit 0
		;;
	test)
		export STATEFULSET_INDEX=0 # example instance number
		echo -e "\e[97;44mRunning unit tests...\e[0m"
		if [[ -d "${BUILD_TARGET_DIR}" ]]; then
			(cd "${BUILD_TARGET_DIR}" && ./tests)
			UNIT_STATUS=$?
		else
			echo "No ${BUILD_TARGET_DIR} directory found. Please build first."
			exit 1
		fi
		if [[ ${UNIT_STATUS} -ne 0 ]]; then
			echo -e "\e[41mUnit tests FAILED.\e[0m"
			exit ${UNIT_STATUS}
		fi
		echo -e "\e[97;44mRunning integration tests...\e[0m"
		PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
		cd "${PROJECT_ROOT}"
		if [[ -f tests/run_integration_tests.sh ]]; then
			bash tests/run_integration_tests.sh -b "${BUILD_TYPE}" -t "${TARGET}"
			INTEGRATION_STATUS=$?
		else
			echo "No integration test script found."
			INTEGRATION_STATUS=0
		fi
		if [[ ${INTEGRATION_STATUS} -ne 0 ]]; then
			echo -e "\e[41mIntegration tests FAILED.\e[0m"
			exit ${INTEGRATION_STATUS}
		fi
		echo -e "\e[32mTest run completed successfully.\e[0m"
		exit 0
		;;
	image)
		echo -e "\e[97;44mBuilding Docker image...\e[0m"
		docker build --no-cache -t fsl .
		echo -e "\e[32mDocker image build completed.\e[0m"
		exit 0
		;;
	all)
		"${0}" clean -b "${BUILD_TYPE}" -t "${TARGET}" "${REMAINING_ARGS[@]}"
		mkdir -p "${BUILD_DIR}"
		"${0}" -b "${BUILD_TYPE}" -t "${TARGET}" "${REMAINING_ARGS[@]}"
		"${0}" test -b "${BUILD_TYPE}" -t "${TARGET}" "${REMAINING_ARGS[@]}"
		exit 0
		;;
	"" | build)
		# Default build process if no positional argument or 'build' is given
		;;
	*)
		echo "Unknown positional argument: ${POS_ARG}"
		print_usage
		exit 1
		;;
	esac

	# If we reach here, do the build
	echo -e "\e[97;44mStarting build process (${BUILD_TYPE}, ${TARGET}) in ${BUILD_TARGET_DIR}...\e[0m"

	mkdir -p "${BUILD_TARGET_DIR}"

	# Debug output
	echo "PWD: $(pwd)"
	echo "Expecting CMakeLists.txt at: $(realpath .)/CMakeLists.txt"
	ls -l .
	ls -l ./CMakeLists.txt || echo 'CMakeLists.txt not found!'

	if [[ "${TARGET}" == "petalinux" ]]; then
		export PETALINUX=1
		local GCC="${HOME}/dev/petalinux/tools/xsct/gnu/aarch64/lin/aarch64-none/x86_64-oesdk-linux/usr/bin/aarch64-xilinx-elf/aarch64-xilinx-elf-gcc"
		local GPP="${HOME}/dev/petalinux/tools/xsct/gnu/aarch64/lin/aarch64-none/x86_64-oesdk-linux/usr/bin/aarch64-xilinx-elf/aarch64-xilinx-elf-g++"
		# local CMAKE_PATH="${HOME}/dev/petalinux/tools/xsct/tps/lnx64/cmake-3.3.2/bin/cmake"
		# export LD_LIBRARY_PATH=
		# source "${HOME}/dev/petalinux/components/yocto/buildtools/environment-setup-x86_64-petalinux-linux"
		source "${HOME}/dev/petalinux/components/yocto/buildtools/environment-setup-aarch64-xilinx-linux"
		# ${CMAKE_PATH} -S . -B "${BUILD_TARGET_DIR}" -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPETALINUX=ON -DCMAKE_C_COMPILER="${GCC}" -DCMAKE_CXX_COMPILER="${GPP}"
		cmake -S . -B "${BUILD_TARGET_DIR}" -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPETALINUX=ON -DCMAKE_C_COMPILER="${GCC}" -DCMAKE_CXX_COMPILER="${GPP}"
	else
		cmake -S . -B "${BUILD_TARGET_DIR}" -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
	fi

	make -C "${BUILD_TARGET_DIR}"
	# Also build tests target if present
	if grep -q 'add_executable *(tests' CMakeLists.txt; then
		make -C "${BUILD_TARGET_DIR}" tests || true
	fi

	echo -e "\e[32mBuild process completed.\e[0m"
}

main "$@"
