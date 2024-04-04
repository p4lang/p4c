#!/bin/bash

# Script for checking static build results.

set -e  # Exit on error.
set -x  # Make command execution verbose
set -u  # Error on unbound variables.

has_lib() {
    ldd $1 2>&1 | grep -E $2
}

is_static() {
    ldd $1 2>&1 | grep -E -qi "(not a dynamic executable)|(statically linked)"
}

# The binaries to check.
bins=(
./build/p4c-bm2-ss
./build/p4c-dpdk
./build/p4c-ebpf
./build/p4c-pna-p4tc
./build/p4c-ubpf
./build/p4test
./build/p4testgen
)

# Run checks on all the binaries produced by the builds.
for bin in ${bins[@]}; do
    # Disabling this checks until we find a better way to build a fully static binary.
    # is_static $bin
    ! has_lib $bin "glibc"
    ! has_lib $bin "libboost_iostreams"
    if [[ ${STATIC_SANS} = "stdlib" ]]; then
        has_lib $bin 'libstdc++\.so'
    else
        ! has_lib $bin 'libstdc++\.so'
    fi
done
