#!/bin/bash

# Script for building in a Docker container on Travis.

set -e  # Exit on error.
set -x  # Make command execution verbose

# Python3 is required for p4c to run, P4C_DEPS would otherwise uninstall it
export P4C_PYTHON3="python3"

export P4C_DEPS="bison \
             build-essential \
             cmake \
             curl \
             flex \
             g++ \
             libboost-dev \
             libboost-graph-dev \
             libboost-iostreams1.58-dev \
             libfl-dev \
             libgc-dev \
             libgmp-dev \
             pkg-config \
             python3-pip \
             python3-setuptools \
             tcpdump"

export P4C_EBPF_DEPS="libpcap-dev \
             libelf-dev \
             zlib1g-dev \
             llvm \
             clang \
             iptables \
             net-tools"

export P4C_RUNTIME_DEPS="cpp \
                     libboost-graph1.58.0 \
                     libboost-iostreams1.58.0 \
                     libgc1c2 \
                     libgmp10 \
                     libgmpxx4ldbl \
                     python3"

export P4C_PIP_PACKAGES="ipaddr \
                          pyroute2 \
                          ply==3.8 \
                          scapy==2.4.4"



apt-get update
apt-get install -y --no-install-recommends \
  ${P4C_DEPS} \
  ${P4C_PYTHON3} \
  ${P4C_EBPF_DEPS} \
  ${P4C_RUNTIME_DEPS} \
  git

# we want to use Python as the default so change the symlinks
ln -sf /usr/bin/python3 /usr/bin/python
ln -sf /usr/bin/pip3 /usr/bin/pip

pip3 install wheel
pip3 install $P4C_PIP_PACKAGES

# Build libbpf for eBPF tests.
cd /p4c
backends/ebpf/build_libbpf
cd -

# We also need to build iproute2 from source to support Ubuntu 16.04 eBPF.
cd /tmp
git clone -b v5.0.0 git://git.kernel.org/pub/scm/linux/kernel/git/shemminger/iproute2.git
cd /tmp/iproute2
./configure
make -j `getconf _NPROCESSORS_ONLN` && \
make install
cd /p4c
rm -rf /tmp/pip
# iproute2-end

# ! ------  BEGIN VALIDATION -----------------------------------------------

function build_gauntlet() {
  # For add-apt-repository.
  apt-get install -y software-properties-common
  # Also pull Gauntlet for translation validation.
  git clone -b stable https://github.com/p4gauntlet/gauntlet /gauntlet
  cd /gauntlet
  git submodule update --init --recursive
  cd -
  # Symlink the parent p4c repository with Gauntlet.
  # TODO: Only use the extension module and its tests.
  rm -rf /gauntlet/modules/p4c
  ln -s /p4c /gauntlet/modules/p4c
  # Symlink the toz3 extension for the p4 compiler.
  mkdir -p /p4c/extensions
  ln -sf /gauntlet/modules/toz3 /p4c/extensions/toz3

  # Install Gauntlet Python dependencies locally.
  # Unfortunately we need Python 3.6, which Xenial (Ubuntu 16.04) does not have.
  add-apt-repository ppa:deadsnakes/ppa
  # The interpreter requires a more modern version of GCC.
  # We install gcc-8 and set it as default. This is also because of Xenial.
  add-apt-repository ppa:ubuntu-toolchain-r/test
  apt update
  apt install -y python3.6
  apt install -y g++-8
  apt install -y gcc-8
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 10
  update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 10
  # The interpreter requires boost filesystem for file management.
  apt install -y libboost-filesystem-dev
  # Install pytest and pytest-xdist to parallelize tests.
  python3.6 -m pip install pytest pytest-xdist==1.34.0
}

# These steps are necessary to validate the correct compilation of the P4C test
# suite programs. See also https://github.com/p4gauntlet/gauntlet.
if [ "$VALIDATION" == "ON" ]; then
  build_gauntlet
fi
# ! ------  END VALIDATION -----------------------------------------------


function build() {
  if [ -e build ]; then /bin/rm -rf build; fi
  mkdir -p build
  cd build

  cmake "$@" ..
  make
}

# Strong optimization.
export CXXFLAGS="${CXXFLAGS} -O3"
# Add the gold linker early to allow sanitization in Ubuntu 16.04
# Context: https://stackoverflow.com/a/50357910
export CXXFLAGS="${CXXFLAGS} -fuse-ld=gold"
# Because of a bug with Ubuntu16.04 and static linking combined with the
# sanitize library, we have to disable this check for static builds.
if [ "$ENABLE_UNIFIED_COMPILATION" != "ON" ]; then
 # Catch null pointer dereferencing.
  export CXXFLAGS="${CXXFLAGS} -fsanitize=null"
fi
# Toggle unified compilation.
CMAKE_FLAGS+="-DENABLE_UNIFIED_COMPILATION=${ENABLE_UNIFIED_COMPILATION} "
# Toggle static builds.
CMAKE_FLAGS+="-DBUILD_STATIC_RELEASE=${BUILD_STATIC_RELEASE} "
# RELEASE should be default, but we want to make sure.
CMAKE_FLAGS+="-DCMAKE_BUILD_TYPE=RELEASE"
build ${CMAKE_FLAGS}

make install
/usr/local/bin/ccache -p -s

if [[ "${IMAGE_TYPE}" == "build" ]] ; then
  apt-get purge -y ${P4C_DEPS} git
  apt-get autoremove --purge -y
  rm -rf /p4c /var/cache/apt/* /var/lib/apt/lists/*
  echo 'Build image ready'

elif [[ "${IMAGE_TYPE}" == "test" ]] ; then
  echo 'Test image ready'

fi
