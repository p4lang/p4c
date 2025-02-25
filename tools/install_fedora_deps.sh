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
    clang \
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
    python3-devel \
    python3-pip \
    python3-thrift \
    readline-devel \
    tcpdump \
    thrift-devel \
    valgrind \
    zlib-devel \
    ninja-build

pip3 install --upgrade pip
pip3 install -r ${P4C_DIR}/requirements.txt

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

# Install BMv2 from source
pushd "${tmp_dir}"
git clone --depth=1 https://github.com/p4lang/behavioral-model
cd behavioral-model
./autogen.sh
./configure --with-pdfixed --with-thrift --with-pi --with-stress-tests --enable-debugger CC="ccache gcc" CXX="ccache g++"
make -j$((`nproc`+1))
make -j$((`nproc`+1)) install-strip
popd

rm -rf "${tmp_dir}"
