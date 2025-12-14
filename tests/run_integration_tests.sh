#!/bin/bash
# Run FSL integration tests end-to-end
# Usage: ./run_integration_tests.sh

set -e

export STATEFULSET_INDEX=${STATEFULSET_INDEX:-0}

cd "$(dirname "$0")/../build-debug"

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

# Run integration tests (from project root)
cd ../
python3 tests/integration_tests.py
TEST_RESULT=$?

# Give FSL time to process and log any final requests
sleep 1

# Kill FSL
kill ${FSL_PID}
wait ${FSL_PID} 2>/dev/null || true

exit ${TEST_RESULT}
