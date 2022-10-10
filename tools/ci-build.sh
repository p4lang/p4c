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
             libboost-dev \
             libboost-graph-dev \
             libboost-iostreams1.71-dev \
             libfl-dev \
             libgc-dev \
             libgmp-dev \
             pkg-config \
             python3 \
             python3-pip \
             python3-setuptools \
             tcpdump"

export P4C_EBPF_DEPS="libpcap-dev \
             libelf-dev \
             zlib1g-dev \
             llvm \
             clang \
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

apt-get update
apt-get install -y --no-install-recommends \
  ${P4C_DEPS} \
  ${P4C_EBPF_DEPS} \
  ${P4C_RUNTIME_DEPS} \
  git

# TODO: Remove this rm -rf line once the ccache memcache config is removed.
rm -rf /usr/local/etc/ccache.conf
/usr/local/bin/ccache --set-config cache_dir=/p4c/.ccache
/usr/local/bin/ccache --set-config max_size=1G

# we want to use Python as the default so change the symlinks
ln -sf /usr/bin/python3 /usr/bin/python
ln -sf /usr/bin/pip3 /usr/bin/pip

pip3 install wheel
pip3 install $P4C_PIP_PACKAGES

# Build libbpf for eBPF tests.
cd /p4c
backends/ebpf/build_libbpf
cd /p4c

function install_ptf_ebpf_test_deps() (
  # Install linux-tools for specified kernels and for current one
  LINUX_TOOLS="linux-tools-`uname -r`"
  for version in $KERNEL_VERSIONS; do
    LINUX_TOOLS+=" linux-tools-$version-generic"
  done
  export P4C_PTF_PACKAGES="gcc-multilib \
                           python3-six \
                           libjansson-dev \
                           $LINUX_TOOLS"
  apt-get install -y --no-install-recommends ${P4C_PTF_PACKAGES}

  git clone --recursive https://github.com/P4-Research/psabpf.git /tmp/psabpf
  cd /tmp/psabpf
  # FIXME: psabpf is under heavy development, later use git tags when it will be ready to use
  git reset --hard edacd0d
  ./build_libbpf.sh
  mkdir build
  cd build
  cmake ..
  make "-j$(nproc)"
  make install
)

if [[ "${INSTALL_PTF_EBPF_DEPENDENCIES}" == "ON" ]] ; then
  install_ptf_ebpf_test_deps
fi

# ! ------  BEGIN VALIDATION -----------------------------------------------

function build_gauntlet() {
  # For add-apt-repository.
  apt-get install -y software-properties-common
  # Symlink the toz3 extension for the p4 compiler.
  mkdir -p /p4c/extensions
  git clone -b stable https://github.com/p4gauntlet/toz3 /p4c/extensions/toz3
  # The interpreter requires boost filesystem for file management.
  apt install -y libboost-filesystem-dev
  # Disable failures on crashes
  CMAKE_FLAGS+="-DVALIDATION_IGNORE_CRASHES=ON "
}

# These steps are necessary to validate the correct compilation of the P4C test
# suite programs. See also https://github.com/p4gauntlet/gauntlet.
if [ "$VALIDATION" == "ON" ]; then
  build_gauntlet
fi
# ! ------  END VALIDATION -----------------------------------------------

# ! ------  BEGIN VALIDATION -----------------------------------------------

function build_tools_deps() {
  # This is needed for P4Testgen.
  apt install -y libboost-filesystem-dev libboost-system-dev wget zip

  # Install a recent version of Z3
  Z3_VERSION="z3-4.8.14"
  Z3_DIST="${Z3_VERSION}-x64-glibc-2.31"

  # Install clang-format for style checks.
  pip3 install --user clang-format

  cd /tmp
  wget https://github.com/Z3Prover/z3/releases/download/${Z3_VERSION}/${Z3_DIST}.zip
  unzip ${Z3_DIST}.zip
  cp -r ${Z3_DIST}/bin/libz3.* /usr/local/lib/
  cp -r ${Z3_DIST}/include/* /usr/local/include/
  cd /p4c
  rm -rf /tmp/${Z3_DIST}
}

# Build the dependencies necessary for the P4Tools platform.
if [ "$ENABLE_TEST_TOOLS" == "ON" ]; then
  build_tools_deps
fi
# ! ------  END TOOLS -----------------------------------------------


function build() {
  if [ -e build ]; then /bin/rm -rf build; fi
  mkdir -p build
  cd build

  cmake "$@" ..
  make
}

# Strong optimization.
export CXXFLAGS="${CXXFLAGS} -O3"
# Toggle unified compilation.
CMAKE_FLAGS+="-DENABLE_UNIFIED_COMPILATION=${ENABLE_UNIFIED_COMPILATION} "
# Toggle static builds.
CMAKE_FLAGS+="-DBUILD_STATIC_RELEASE=${BUILD_STATIC_RELEASE} "
# Toggle whether to use GMP or boost::multiprecision
CMAKE_FLAGS+="-DENABLE_GMP=${ENABLE_GMP} "
# Toggle the installation of the tools back end.
CMAKE_FLAGS+="-DENABLE_TEST_TOOLS=${ENABLE_TEST_TOOLS} "
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
