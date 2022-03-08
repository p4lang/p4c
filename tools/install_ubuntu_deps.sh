#!/bin/bash

# Script for building in a Docker container on Travis.

set -e  # Exit on error.
set -x  # Make command execution verbose

export P4C_DEPS="bison \
             build-essential \
             cmake \
             curl \
             flex \
             g++ \
	     llvm \
             clang \
             libboost-dev \
             libboost-graph-dev \
             libboost-iostreams1.71-dev \
             libfl-dev \
             libgc-dev \
             libgmp-dev \
	     libgrpc++-dev \
	     libgrpc-dev \
             pkg-config \
             python3 \
             python3-pip \
             python3-setuptools \
             tcpdump \
	     protobuf-compiler"

export P4C_EBPF_DEPS="libpcap-dev \
             libelf-dev \
             zlib1g-dev \
             iproute2 \
             iptables \
             net-tools"

export P4C_RUNTIME_DEPS="cpp \
                     libboost-graph1.71.0 \
                     libboost-iostreams1.71.0 \
                     libgc1c2 \
                     libgmp10 \
                     libgmpxx4ldbl \
                     python3"

# use scapy 2.4.5, which is the version on which ptf depends
export P4C_PIP_PACKAGES="ipaddr \
                          pyroute2 \
                          ply==3.8 \
                          scapy==2.4.5"

sudo apt-get update

sudo apt-get install -y --no-install-recommends \
  ${P4C_DEPS} \
  ${P4C_EBPF_DEPS} \
  ${P4C_RUNTIME_DEPS} \
  git

# we want to use Python as the default so change the symlinks
sudo ln -sf /usr/bin/python3 /usr/bin/python
sudo ln -sf /usr/bin/pip3 /usr/bin/pip

sudo pip3 install wheel
sudo pip3 install $P4C_PIP_PACKAGES

tmp_dir=$(mktemp -d -t ci-XXXXXXXXXX)
pushd "${tmp_dir}"
if [ ! -d behavioral-model ];then
git clone https://github.com/p4lang/behavioral-model.git
fi
cd behavioral-model
sudo ./install_deps.sh
sudo ./autogen.sh
./configure
make -j
sudo make install-strip
popd
rm -rf "${tmp_dir}"
# ! ------  BEGIN VALIDATION -----------------------------------------------
function build_gauntlet() {
  # For add-apt-repository.
  sudo apt-get install -y software-properties-common
  # Symlink the toz3 extension for the p4 compiler.
  mkdir -p extensions
  git clone -b stable https://github.com/p4gauntlet/toz3 extensions/toz3
  # The interpreter requires boost filesystem for file management.
  sudo apt install -y libboost-filesystem-dev
  # Disable failures on crashes
  CMAKE_FLAGS+="-DVALIDATION_IGNORE_CRASHES=ON "
}
build_gauntlet
# ! ------  END VALIDATION -----------------------------------------------
