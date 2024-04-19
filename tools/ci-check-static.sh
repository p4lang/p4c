#!/bin/bash

# Script for checking semi-static build results.

# No set -e. Do not exit on error, errors are handled explictly.
set -x  # Make command execution verbose
set -u  # Error on unbound variables.

# Usage: check EXPECTED REAL_RET_CODE BINARY MESSAGE
check() {
    if [[ $2 = 0 ]]; then
        expected='true';
    else
        expected='false';
    fi
    if [[ $1 != $expected ]]; then
        echo "Binary dynamic dependency check failed for ${3}: ${4}."
        ldd $3
        exit 1
    fi
}

# Usage: has_lib EXPECTED BINARY LIB_PATTERN
# Does string match, not regex match.
has_lib() {
    ldd $2 2>&1 | grep -F -qi $3
    check $1 $? $2 "has_lib ${3} expected to be ${1}"
}

# Usage: is_static EXPECTED BINARY
is_static() {
    ldd $2 2>&1 | grep -E -qi "(not a dynamic executable)|(statically linked)"
    check $1 $? $2 "is_static expected to be ${1}"
}

# The binaries to check -- arguments except for $0.
bins=${@:1}

# Run checks on all the binaries produced by the builds.
for bin in ${bins[@]}; do
    # Disabling this checks until we find a better way to build a fully static binary.
    # is_static true $bin
    is_static false $bin
    has_lib true $bin "libc"
    has_lib false $bin "libboost_iostreams"
    if [[ ${STATIC_BUILD_WITH_DYNAMIC_STDLIB} = "ON" ]]; then
        has_lib true $bin 'libstdc++'
    else
        has_lib false $bin 'libstdc++'
    fi
done
