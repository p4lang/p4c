#!/bin/bash

set -e  # Exit on error.
set -x  # Make command execution verbose

if [ -z "$1" ]; then
    echo "- Missing mandatory argument: P4C_DIR"
    echo " - Usage: source dpdk-ci-build.sh <P4C_DIR> <IPDK_RECIPE> <SDE_INSTALL> <DEPEND_INSTALL>"
    return 0
fi

if [ -z "$2" ]; then
    echo "- Missing mandatory argument: IPDK_RECIPE"
    echo " - Usage: source dpdk-ci-build.sh <P4C_DIR> <IPDK_RECIPE> <SDE_INSTALL> <DEPEND_INSTALL>"
    return 0
fi

if [ -z "$3" ]; then
    echo "- Missing mandatory argument: SDE_INSTALL"
    echo " - Usage: source dpdk-ci-build.sh <P4C_DIR> <IPDK_RECIPE> <SDE_INSTALL> <DEPEND_INSTALL>"
    return 0
fi

if [ -z "$4" ]; then
    echo "- Missing mandatory argument: DEPEND_INSTALL"
    echo " - Usage: source dpdk-ci-build.sh <P4C_DIR> <IPDK_RECIPE> <SDE_INSTALL> <DEPEND_INSTALL>"
    return 0
fi

export P4C_DIR=$1
export IPDK_RECIPE=$2
export SDE_INSTALL=$3
export DEPEND_INSTALL=$4



# Update SDE libraries for infrap4d; commands are copied from  ipdk.recipe/scripts/dpdk/dpdk-ci-build.sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDE_INSTALL/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDE_INSTALL/lib64


 #if OS =Fedora:   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDE_INSTALL/lib64
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SDE_INSTALL/lib/x86_64-linux-gnu


# Update IPDK RECIPE libraries
export LD_LIBRARY_PATH=$IPDK_RECIPE/install/lib:$IPDK_RECIPE/install/lib64:$LD_LIBRARY_PATH
export PATH=$IPDK_RECIPE/install/bin:$IPDK_RECIPE/install/sbin:$PATH

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib64

# Update Dependent libraries
export LD_LIBRARY_PATH=$DEPEND_INSTALL/lib:$DEPEND_INSTALL/lib64:$LD_LIBRARY_PATH
export PATH=$DEPEND_INSTALL/bin:$DEPEND_INSTALL/sbin:$PATH
export LIBRARY_PATH=$DEPEND_INSTALL/lib:$DEPEND_INSTALL/lib64:$LIBRARY_PATH

echo ""
echo ""
echo "Updated Environment Variables ..."
echo "SDE_INSTALL: $SDE_INSTALL"
echo "LIBRARY_PATH: $LIBRARY_PATH"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "PATH: $PATH"
echo ""

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

apt-get update
apt-get install -y --no-install-recommends  ${P4C_DEPS}
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
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
"

mkdir build
cd build
cmake .. ${CMAKE_FLAGS}
make "-j$(nproc)"

set +e

