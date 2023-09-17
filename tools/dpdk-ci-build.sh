#!/bin/bash

set -e  # Exit on error.
set -x  # Make command execution verbose

THIS_DIR=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )
P4C_DIR=$(readlink -f ${THIS_DIR}/..)

if [ -z "$1" ]; then
    echo "- Missing mandatory argument: IPDK_INSTALL_DIR"
    echo " - Usage: source dpdk-ci-build.sh <IPDK_INSTALL_DIR>"
    return 0
fi

export IPDK_INSTALL_DIR=$1

# Install P4C deps and build p4c-dpdk
cd $P4C_DIR
P4C_DEPS="bison \
          build-essential \
          ccache \
          cmake \
          flex \
          g++ \
          git \
          lld \
          libboost-dev \
          libboost-graph-dev \
          libboost-iostreams-dev \
          libfl-dev \
          libgc-dev \
          pkg-config \
          python3 \
          python3-pip \
          python3-setuptools \
          tcpdump \
          tcpreplay \
          python3-netaddr"

sudo apt-get update
sudo apt-get install -y --no-install-recommends  ${P4C_DEPS}
pip3 install --upgrade pip
pip3 install -r ${P4C_DIR}/requirements.txt
pip3 install git+https://github.com/p4lang/p4runtime-shell.git

# Build P4C
CMAKE_FLAGS="-DCMAKE_UNITY_BUILD=ON"
CMAKE_FLAGS+="
  -DENABLE_BMV2=OFF \
  -DENABLE_EBPF=OFF \
  -DENABLE_UBPF=OFF \
  -DENABLE_GTESTS=OFF \
  -DENABLE_P4TEST=OFF \
  -DENABLE_P4TC=OFF \
  -DENABLE_P4C_GRAPHS=OFF \
  -DENABLE_PROTOBUF_STATIC=OFF \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DENABLE_TEST_TOOLS=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DIPDK_INSTALL_DIR=${IPDK_INSTALL_DIR}
"

mkdir build
cd build
cmake .. ${CMAKE_FLAGS}
make "-j$(nproc)"

set +e

