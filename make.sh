#!/bin/bash

set -e

function main() {
	print_usage() {
		echo "Usage: $0 [clean|test|all|image] [-b|--build debug|release] [-t|--target linux|petalinux] [-h|--help]"
		echo "  clean         Remove build folder"
		echo "  test          Run unit and integration tests"
		echo "  all           Run clean, build, and test"
		echo "  image         Build the Docker image (fsl-app)"
		echo "  -b, --build   Build type: debug (default) or release"
		echo "  -t, --target  Target platform: linux (default) or petalinux"
		echo "  -h, --help    Show this help message"
	}
	BUILD_TYPE="debug"
	BUILD_DIR="build"
	TARGET="linux"

	# Handle positional arguments (clean, test, all, image)
	if [[ "$1" == "clean" ]]; then
		# Compute BUILD_TARGET_DIR as build/<target>/<build_type> for cleaning
		BUILD_TARGET_DIR="${BUILD_DIR}/${TARGET}/$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
		echo -e "\e[97;44mCleaning build folder: ${BUILD_TARGET_DIR}...\e[0m"
		rm -rf "${BUILD_TARGET_DIR}"
		echo -e "\e[32mClean completed.\e[0m"
		exit 0
	elif [[ "$1" == "test" ]]; then
		# Parse options to set BUILD_TYPE, TARGET, etc. for this test run
		shift
		while [[ $# -gt 0 ]]; do
			key="$1"
			case $key in
			-b | --build)
				if [[ "$2" == "release" || "$2" == "release" ]]; then
					BUILD_TYPE="release"
				elif [[ "$2" == "debug" || "$2" == "debug" ]]; then
					BUILD_TYPE="debug"
				else
					echo "Unknown build type: $2. Use 'debug' or 'release'."
					exit 1
				fi
				shift
				shift
				;;
			-t | --target)
				if [[ "$2" == "linux" || "$2" == "Linux" ]]; then
					TARGET="linux"
				elif [[ "$2" == "petalinux" || "$2" == "PetaLinux" || "$2" == "petalinux" ]]; then
					TARGET="petalinux"
				else
					echo "Unknown target: $2. Use 'linux' or 'petalinux'."
					exit 1
				fi
				shift
				shift
				;;
			*)
				shift
				;;
			esac
		done
		BUILD_TARGET_DIR="${BUILD_DIR}/${TARGET}/$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
		export BUILD_TARGET_DIR
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
	elif [[ "${1}" == "image" ]]; then
		echo -e "\e[97;44mBuilding Docker image...\e[0m"
		docker build -t fsl .
		echo -e "\e[32mDocker image build completed.\e[0m"
		exit 0
	elif [[ "${1}" == "all" ]]; then
		shift
		"${0}" clean
		mkdir -p "${BUILD_DIR}"
		"${0}" "$@"
		"${0}" test "$@"
		exit 0
	fi

	# Parse options
	while [[ $# -gt 0 ]]; do
		key="${1}"
		case ${key} in
		-b | --build)
			if [[ "${2}" == "release" || "${2}" == "release" ]]; then
				BUILD_TYPE="release"
			elif [[ "${2}" == "debug" || "${2}" == "debug" ]]; then
				BUILD_TYPE="debug"
			else
				echo "Unknown build type: ${2}. Use 'debug' or 'release'."
				exit 1
			fi
			shift # past argument
			shift # past value
			;;
		-t | --target)
			if [[ "${2}" == "linux" || "${2}" == "Linux" ]]; then
				TARGET="linux"
			elif [[ "${2}" == "petalinux" || "${2}" == "PetaLinux" || "${2}" == "petalinux" ]]; then
				TARGET="petalinux"
			else
				echo "Unknown target: ${2}. Use 'linux' or 'petalinux'."
				exit 1
			fi
			shift # past argument
			shift # past value
			;;
		-h | --help)
			print_usage
			exit 0
			;;
		*) # unknown option
			echo "Unknown option: ${1}"
			print_usage
			exit 1
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

	# Print in white on blue background
	echo -e "\e[97;44mStarting build process (${BUILD_TYPE}, ${TARGET}) in ${BUILD_TARGET_DIR}...\e[0m"

	mkdir -p "${BUILD_TARGET_DIR}"
	cd "${BUILD_TARGET_DIR}"

	# Debug output
	echo "PWD: $(pwd)"
	echo "Expecting CMakeLists.txt at: $(realpath ../../../)/CMakeLists.txt"
	ls -l ../../..
	ls -l ../../../CMakeLists.txt || echo 'CMakeLists.txt not found!'

	if [[ "${TARGET}" == "petalinux" ]]; then
		cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DPETALINUX=ON ../../../
	else
		cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ../../../
	fi

	make
	# Also build tests target if present
	if grep -q 'add_executable *(tests' ../../CMakeLists.txt; then
		make tests || true
	fi
	cd ../..

	# Print in green
	echo -e "\e[32mBuild process completed.\e[0m"
}

main "$@"
