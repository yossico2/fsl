#!/bin/bash
# Run FSL integration tests end-to-end
# Usage: ./run_integration.sh

set -e

cd "$(dirname "$0")/../build-debug"

# Ensure logs directory exists under project root
LOGDIR="../logs"
mkdir -p "${LOGDIR}"

# Remove previous log file if it exists
if [ -f "${LOGDIR}/fsl-1.log" ]; then
	rm -f "${LOGDIR}/fsl-1.log"
fi

# Start FSL in background, redirect output to logs/fsl-1.log
./fsl >"${LOGDIR}/fsl-1.log" 2>&1 &
FSL_PID=$!

# Wait for FSL to initialize
sleep 1

# Run integration tests (from project root)
cd ../
python3 tests/integration_test.py
TEST_RESULT=$?

# Give FSL time to process and log any final requests
sleep 1

# Kill FSL
kill ${FSL_PID}
wait ${FSL_PID} 2>/dev/null || true

exit ${TEST_RESULT}
