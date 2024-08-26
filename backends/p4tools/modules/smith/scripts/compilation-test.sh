#!/bin/bash

set -e # Exit on error.

# List of known bugs.
# Catch bugs generically.
KNOWN_BUGS=(
    "\*.*not implemented" # DPDK failure.
    ".*| + |.*not implemented" # DPDK failure.
    ".*| - |.*not implemented" # DPDK failure.
    ".*true.*not implemented" # DPDK failure.
    "Cannot evaluate initializer for constant"
    "Null expr"
    "error: retval_1: declaration not found"  # V1model failure.
)

# Function to check if an error is triggered by a known bug.
is_known_bug() {
    local error_msg="$1"
    for bug in "${KNOWN_BUGS[@]}"; do
        if echo "$error_msg" | grep -q "$bug"; then
            echo "Bug encountered: $bug"
            return 0
        fi
    done
    return 1
}

if [ -z "$1" ]; then
    echo "- Missing mandatory argument: NUM_ITERATIONS"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

if [ -z "$2" ]; then
    echo "- Missing mandatory argument: SMITH_BIN"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

if [ -z "$3" ]; then
    echo "- Missing mandatory argument: COMPILER_BIN"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

if [ -z "$4" ]; then
    echo "- Missing mandatory argument: TEST_DIR"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

if [ -z "$5" ]; then
    echo "- Missing mandatory argument: ARCH"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

if [ -z "$6" ]; then
    echo "- Missing mandatory argument: TARGET"
    echo " - Usage: source compilation-test.sh <NUM_ITERATIONS> <SMITH_BIN> <COMPILER_BIN> <TEST_DIR> <ARCH> <TARGET>"
    return 1
fi

NUM_ITERATIONS=$1
SMITH_BIN=$2
COMPILER_BIN=$3
TEST_DIR=$4
ARCH=$5
TARGET=$6

TMP_DIR=$(mktemp -d -p $TEST_DIR -t tmpXXXX)
for i in $(seq 1 $NUM_ITERATIONS); do
    echo "Generating program $i in $TMP_DIR"
    # Generate different programs with different seeds (to inspect the behavior of the generation and compilation process in a finer/smaller granularity).
    echo "$SMITH_BIN --arch $ARCH --target $TARGET --seed $i $TMP_DIR/out_$i.p4"
    $SMITH_BIN --arch $ARCH --target $TARGET --seed $i $TMP_DIR/out_$i.p4
    # TODO: Do not compile until we have stabilized.

    # If the compilation fails, check if it is triggered by a known bug.
    # If it is the case, continue with the next iteration.
    # Otherwise, exit with an error.
    CMD="$COMPILER_BIN $TMP_DIR/out_$i.p4"
    echo "$CMD"
    if ! output=$($CMD 2>&1); then
        if is_known_bug "$output"; then
            echo "Continue, as the compilation error is triggered by a documented bug: $output"
            continue
        else
            echo "Abort, as the compilation error is triggered by an undocumented bug: $output"
            exit 1
        fi
    else
        echo "$output"
    fi
done
