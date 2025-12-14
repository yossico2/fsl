#!/bin/bash

set -e

function main() {
	print_usage() {
		echo "Usage: $0 [clean|test|all|image] [-b|--build debug|release] [-t|--target linux|petalinux] [-h|--help]"
		echo "  clean         Remove build-debug and build-release folders"
		echo "  test          Run unit and integration tests"
		echo "  all           Run clean, build, and test"
		echo "  image         Build the Docker image (fsl-app)"
		echo "  -b, --build   Build type: debug (default) or release"
		echo "  -t, --target  Target platform: linux (default) or petalinux"
		echo "  -h, --help    Show this help message"
	}
	BUILD_TYPE="Debug"
	BUILD_DIR="build-debug"
	TARGET="linux"

	# Handle positional arguments (clean, test, all, image)
	if [[ "$1" == "clean" ]]; then
		echo -e "\e[97;44mCleaning build folders...\e[0m"
		rm -rf build-debug build-release
		echo -e "\e[32mClean completed.\e[0m"
		exit 0
	elif [[ "$1" == "test" ]]; then
		export STATEFULSET_INDEX=0 # example instance number
		echo -e "\e[97;44mRunning unit tests...\e[0m"
		if [[ -d build-debug ]]; then
			(cd build-debug && ./tests)
			UNIT_STATUS=$?
		else
			echo "No build-debug directory found. Please build first."
			exit 1
		fi
		if [[ $UNIT_STATUS -ne 0 ]]; then
			echo -e "\e[41mUnit tests FAILED.\e[0m"
			exit $UNIT_STATUS
		fi
		echo -e "\e[97;44mRunning integration tests...\e[0m"
		if [[ -f tests/run_integration_tests.sh ]]; then
			bash tests/run_integration_tests.sh
			INTEGRATION_STATUS=$?
		else
			echo "No integration test script found."
			INTEGRATION_STATUS=0
		fi
		if [[ $INTEGRATION_STATUS -ne 0 ]]; then
			echo -e "\e[41mIntegration tests FAILED.\e[0m"
			exit $INTEGRATION_STATUS
		fi
		echo -e "\e[32mTest run completed successfully.\e[0m"
		exit 0
	elif [[ "$1" == "image" ]]; then
		echo -e "\e[97;44mBuilding Docker image...\e[0m"
		docker build -t fsl .
		echo -e "\e[32mDocker image build completed.\e[0m"
		exit 0
	elif [[ "$1" == "all" ]]; then
		"$0" clean
		"$0"
		"$0" test
		exit 0
	fi

	# Parse options
	while [[ $# -gt 0 ]]; do
		key="$1"
		case $key in
		-b | --build)
			if [[ "$2" == "release" || "$2" == "Release" ]]; then
				BUILD_TYPE="Release"
				BUILD_DIR="build-release"
			elif [[ "$2" == "debug" || "$2" == "Debug" ]]; then
				BUILD_TYPE="Debug"
				BUILD_DIR="build-debug"
			else
				echo "Unknown build type: $2. Use 'debug' or 'release'."
				exit 1
			fi
			shift # past argument
			shift # past value
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
			shift # past argument
			shift # past value
			;;
		-h | --help)
			print_usage
			exit 0
			;;
		*) # unknown option
			echo "Unknown option: $1"
			print_usage
			exit 1
			;;
		esac
	done

	# Print in white on blue background
	echo -e "\e[97;44mStarting build process ($BUILD_TYPE, $TARGET)...\e[0m"

	mkdir -p "$BUILD_DIR"
	cd "$BUILD_DIR"

	if [[ "$TARGET" == "petalinux" ]]; then
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DPETALINUX=ON ..
	else
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
	fi

	make
	# Also build tests target if present
	if grep -q 'add_executable *(tests' ../CMakeLists.txt; then
		make tests || true
	fi
	cd ..

	# Print in green
	echo -e "\e[32mBuild process completed.\e[0m"
}

main "$@"
