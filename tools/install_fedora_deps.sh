#!/bin/bash

set -e

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
    gc-devel \
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
    python3-thrift \
    readline-devel \
    tcpdump \
    thrift-devel \
    valgrind \
    zlib-devel \
    ninja-build

sudo pip3 install ply ptf scapy==2.5.0 wheel

# Install dependencies for the BMv2 PTF runner and P4Runtime.
sudo pip3 install --upgrade protobuf==3.20.3
sudo pip3 install --upgrade googleapis-common-protos==1.61.0
sudo pip3 install --upgrade grpcio==1.59.2

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
