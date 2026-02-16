#!/bin/bash

# Script for building P4C on Fedora for continuous integration builds.

set -e  # Exit on error.
set -x  # Make command execution verbose

THIS_DIR=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )
P4C_DIR=$(readlink -f ${THIS_DIR}/..)

# In Docker builds, sudo is not available. So make it a noop.
if [ "$IN_DOCKER" == "TRUE" ]; then
  echo "Executing within docker container."
  function sudo() { command "$@"; }
fi

sudo dnf install -y -q \
    awk \
    automake \
    bison \
    boost-devel \
    boost-filesystem \
    boost-graph \
    boost-iostreams \
    boost-program-options \
    boost-system \
    boost-test \
    boost-thread \
    ccache \
    clang-15 \
    cmake \
    cpp \
    elfutils-libelf-devel \
    flex \
    g++ \
    git \
    gmp-devel \
    grpc-devel \
    grpc-plugins \
    iproute \
    iptables-legacy \
    libevent-devel \
    libfl-devel \
    libpcap-devel \
    libtool \
    llvm \
    make \
    nanomsg-devel \
    net-tools \
    openssl-devel \
    pkg-config \
    procps-ng \
    python3 \
    python3-pip \
    python3-virtualenv \
    python3-thrift \
    uv \
    curl \
    readline-devel \
    tcpdump \
    thrift-devel \
    valgrind \
    zlib-devel \
    glibc-devel.i686 \
    ninja-build

# Set up uv for Python dependency management.
uv sync

MAKEFLAGS="-j$(nproc)"
export MAKEFLAGS

# Install PI from source
tmp_dir=$(mktemp -d -t ci-XXXXXXXXXX)

pushd "${tmp_dir}"
git clone --recurse-submodules --depth=1 https://github.com/p4lang/PI
cd PI
./autogen.sh
./configure --with-proto
make -j$((`nproc`+1))
make -j$((`nproc`+1)) install
popd

# Install BMv2 from source via the shared CMake-based helper.
BMV2_INSTALL_ARGS=(
  --work-dir "${tmp_dir}"
)
if [[ -n "${BMV2_REF:-}" ]]; then
  BMV2_INSTALL_ARGS+=(--ref "${BMV2_REF}")
fi
"${THIS_DIR}/install_bmv2_from_source.sh" "${BMV2_INSTALL_ARGS[@]}"

git clone https://github.com/libbpf/libbpf/ -b v1.5.0 ${P4C_DIR}/backends/tc/runtime/libbpf
${P4C_DIR}/backends/tc/runtime/build-libbpf

rm -rf "${tmp_dir}"
