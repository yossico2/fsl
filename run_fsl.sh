#!/bin/bash

print_usage() {
	echo "Usage: $0 [instance] [-d] [-nd] [-h|--help]"
	echo "  instance    Instance number to run (default: 1)"
	echo "  -d          Run container in the background (detached mode)"
	echo "  -nd         Run FSL directly (no Docker)"
	echo "  -h, --help  Show this help message"
	echo ""
	echo "Environment variables:"
	echo "  NO_DOCKER=true   Run FSL directly (no Docker), equivalent to -nd"
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
	print_usage
	exit 0
fi

function main() {
	local detach=""
	local instance=""
	local run_no_docker=0

	# parse args in any order
	# 1. If -i/--instance or a positional instance is provided, it is used.
	# 2. If not, and STATEFULSET_INDEX is defined, it is used.
	# 3. Otherwise, it defaults to 0.
	local arg_instance=""
	for ((i = 1; i <= $#; i++)); do
		arg="${!i}"
		if [[ "$arg" == "-d" ]]; then
			detach="-d"
		elif [[ "$arg" == "-nd" ]]; then
			run_no_docker=1
		elif [[ "$arg" == "-i" || "$arg" == "--instance" ]]; then
			next_idx=$((i + 1))
			if [[ $next_idx -le $# ]]; then
				arg_instance="${!next_idx}"
			fi
		elif [[ "$arg" =~ ^[0-9]+$ && -z "$arg_instance" ]]; then
			arg_instance="$arg"
		fi
	done

	# Determine instance priority
	if [[ -n "$arg_instance" ]]; then
		instance="$arg_instance"
	elif [[ -n "${STATEFULSET_INDEX}" ]]; then
		instance="${STATEFULSET_INDEX}"
	else
		instance=""
	fi

	# Check NO_DOCKER env var
	if [[ "${NO_DOCKER}" == "true" ]]; then
		run_no_docker=1
	fi

	local prev_pids=
	if [[ ${run_no_docker} -eq 1 ]]; then
		logdir="${HOME}/dev/logs"
		mkdir -p "${logdir}"

		local project_dir=
		local env_path
		env_path="$(dirname "$0")/env.sh"
		if [[ -f "${env_path}" ]]; then
			# source env.sh to get BUILD_TARGET_DIR
			# shellcheck disable=SC1090
			source "${env_path}"
			project_dir="${BUILD_TARGET_DIR}"
		else
			project_dir="${HOME}/dev/elar/elar_fsl/build/linux/debug"
		fi

		cd "${project_dir}" || exit 1

		# Check for previous fsl for this instance and fail if found
		if [[ -n "${instance}" ]]; then
			prev_pids=$(pgrep -f "[.]\/fsl[ ]${instance}")
		else
			prev_pids=$(pgrep -f "[.]\/fsl[ ]*$")
		fi
		if [[ -n "${prev_pids}" ]]; then
			echo -e "\033[0;31mError: Previous fsl instance(s) running (pid: ${prev_pids})\033[0m" >&2
			cd - &>/dev/null || exit 1
			exit 1
		fi

		if [[ -n "${detach}" ]]; then
			if [[ -n "${instance}" ]]; then
				nohup ./fsl "${instance}" >"${logdir}/fsl-${instance}.log" 2>&1 &
			else
				nohup ./fsl >"${logdir}/fsl-noinstance.log" 2>&1 &
			fi
			process_pid=$!
			sleep 0.5
			if ! ps -p "${process_pid}" >/dev/null 2>&1; then
				echo -e "\033[0;31mError: Failed to start fsl in background\033[0m" >&2
				cd - &>/dev/null || exit 1
				exit 1
			fi
			cd - &>/dev/null || exit 1
			if [[ -n "${instance}" ]]; then
				echo "Started fsl instance ${instance} in background (PID ${process_pid}). Output: ${logdir}/fsl-${instance}.log" >&2
			else
				echo "Started fsl with no instance in background (PID ${process_pid}). Output: ${logdir}/fsl-noinstance.log" >&2
			fi
		else
			if [[ -n "${instance}" ]]; then
				./fsl "${instance}" | tee "${logdir}/fsl-${instance}.log"
				cd - &>/dev/null || exit 1
				echo "Started fsl instance ${instance} in foreground. Output: ${logdir}/fsl-${instance}.log" >&2
			else
				./fsl | tee "${logdir}/fsl-noinstance.log"
				cd - &>/dev/null || exit 1
				echo "Started fsl with no instance in foreground. Output: ${logdir}/fsl-noinstance.log" >&2
			fi
		fi
	else
		docker run --rm ${detach} -it --network=host \
			--name "fsl-${instance}" \
			-e STATEFULSET_INDEX="${instance}" \
			fsl
	fi
}

main "$@"
