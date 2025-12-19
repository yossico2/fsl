#!/bin/bash

PIDS=$(pgrep -f "./fsl")
if [[ -n "${PIDS}" ]]; then
	# shellcheck disable=SC2086
	kill ${PIDS}
fi
