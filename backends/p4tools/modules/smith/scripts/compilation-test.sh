#!/bin/bash

set -e  # Exit on error.

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

for i in $(seq 1 $NUM_ITERATIONS); do
    echo "Generating program $i"
    $SMITH_BIN --target $ARCH --arch $TARGET --seed $i $TEST_DIR/out.p4
    # TODO: Do not compile until we have stabilized.
    # $COMPILER_BIN $TEST_DIR/out.p4
done
