#!/bin/bash

# Script for building in a Docker container on Travis.

set -e  # Exit on error.
set -x  # Make command execution verbose

THIS_DIR=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )
P4C_DIR=$(readlink -f ${THIS_DIR}/..)

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
: "${MAKEFLAGS:=-j2}"
# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
: "${IMAGE_TYPE:=build}"
# Whether to do a unity build.
: "${CMAKE_UNITY_BUILD:=ON}"
# Whether to enable translation validation
: "${VALIDATION:=OFF}"
# This creates a release build that includes link time optimization and links
# all libraries statically.
: "${BUILD_STATIC_RELEASE:=OFF}"
# No questions asked during package installation.
: "${DEBIAN_FRONTEND:=noninteractive}"
# Whether to install dependencies required to run PTF-ebpf tests
: "${INSTALL_PTF_EBPF_DEPENDENCIES:=OFF}"
# Whether to build the P4Tools back end and platform.
: "${ENABLE_TEST_TOOLS:=OFF}"

# Whether to treat warnings as errors.
: "${ENABLE_WERROR:=ON}"
# Compile with Clang compiler
: "${COMPILE_WITH_CLANG:=OFF}"
# Compile with sanitizers (UBSan, ASan)
: "${ENABLE_SANITIZERS:=OFF}"
# Only execute the steps necessary to successfully run CMake.
: "${CMAKE_ONLY:=OFF}"
# Build with -ftrivial-auto-var-init=pattern to catch more bugs caused by
# uninitialized variables.
: "${BUILD_AUTO_VAR_INIT_PATTERN:=OFF}"

. /etc/lsb-release

P4C_DEPS="bison \
          build-essential \
          ccache \
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
          tcpdump"

P4C_EBPF_DEPS="libpcap-dev \
               libelf-dev \
               zlib1g-dev \
               llvm \
               clang \
               iproute2 \
               iptables \
               net-tools"

# In Docker builds, sudo is not available. So make it a noop.
if [ "$IN_DOCKER" == "TRUE" ]; then
  echo "Executing within docker container."
  function sudo() { command "$@"; }
fi

# TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" == "18.04" ]] ; then
  P4C_RUNTIME_DEPS_BOOST="libboost-graph1.65.1 libboost-iostreams1.65.1"
else
  P4C_RUNTIME_DEPS_BOOST="libboost-graph1.7* libboost-iostreams1.7*"
fi

P4C_RUNTIME_DEPS="cpp \
                  ${P4C_RUNTIME_DEPS_BOOST} \
                  libgc1* \
                  libgmp-dev \
                  python3"

# TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" == "18.04" ]] || [[ "$(which simple_switch 2> /dev/null)" != "" ]] ; then
  # Use GCC 9 from https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test
  sudo apt-get update && sudo apt-get install -y software-properties-common
  sudo add-apt-repository -uy ppa:ubuntu-toolchain-r/test
  P4C_DEPS+=" gcc-9 g++-9"
  export CC=gcc-9
  export CXX=g++-9
else
  sudo apt-get update && sudo apt-get install -y curl gnupg
  echo "deb https://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/ /" | sudo tee /etc/apt/sources.list.d/home:p4lang.list
  curl -L "https://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/Release.key" | sudo apt-key add -
  # Try to avoid certificate errors.
  sudo apt install ca-certificates
  P4C_DEPS+=" p4lang-bmv2"
fi

# TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" != "18.04" ]] ; then
  P4C_DEPS+=" cmake"
fi

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ${P4C_DEPS} \
  ${P4C_EBPF_DEPS} \
  ${P4C_RUNTIME_DEPS}

sudo pip3 install --upgrade pip
sudo pip3 install -r ${P4C_DIR}/requirements.txt

# TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" == "18.04" ]] ; then
  ccache --set-config cache_dir=.ccache
  # For Ubuntu 18.04 install the pypi-supplied version of cmake instead.
  sudo pip3 install cmake==3.16.3
fi
ccache --set-config max_size=1G


# Install add-ons to communicate with simple_switch_grpc via P4Runtime.
# These packages are necessary because of a protobuf version mismatch in more recent Ubuntu distributions.
if [[ "${DISTRIB_RELEASE}" == "22.04" ]] ; then
  sudo pip3 install --upgrade protobuf==3.20.1
  sudo pip3 install --upgrade googleapis-common-protos==1.50.0
  sudo pip3 install --upgrade grpcio==1.51.1
fi

# Build libbpf for eBPF tests.
pushd ${P4C_DIR}
backends/ebpf/build_libbpf
popd

# ! ------  BEGIN PTF_EBPF -----------------------------------------------
function install_ptf_ebpf_test_deps() (
  P4C_PTF_PACKAGES="gcc-multilib \
                           python3-six \
                           libgmp-dev \
                           libjansson-dev"
  sudo apt-get install -y --no-install-recommends ${P4C_PTF_PACKAGES}

  git clone --depth 1 --recursive --branch v0.3.1 https://github.com/NIKSS-vSwitch/nikss /tmp/nikss
  pushd /tmp/nikss
  ./build_libbpf.sh
  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release ..
  make "-j$(nproc)"
  sudo make install

  # install bpftool
  git clone --recurse-submodules https://github.com/libbpf/bpftool.git /tmp/bpftool
  cd /tmp/bpftool/src
  make "-j$(nproc)"
  sudo make install
  popd
)
# ! ------  END PTF_EBPF -----------------------------------------------

if [[ "${INSTALL_PTF_EBPF_DEPENDENCIES}" == "ON" ]] ; then
  install_ptf_ebpf_test_deps
fi

# ! ------  BEGIN VALIDATION -----------------------------------------------
function build_gauntlet() {
  # For add-apt-repository.
  sudo apt-get install -y software-properties-common
  # Symlink the toz3 extension for the p4 compiler.
  mkdir -p ${P4C_DIR}/extensions
  git clone -b stable https://github.com/p4gauntlet/toz3 extensions/toz3
  # The interpreter requires boost filesystem for file management.
  sudo apt-get install -y libboost-filesystem-dev
  # Disable failures on crashes
  CMAKE_FLAGS+="-DVALIDATION_IGNORE_CRASHES=ON "
}

# These steps are necessary to validate the correct compilation of the P4C test
# suite programs. See also https://github.com/p4gauntlet/gauntlet.
if [ "$VALIDATION" == "ON" ]; then
  build_gauntlet
fi
# ! ------  END VALIDATION -----------------------------------------------

# ! ------  BEGIN P4TOOLS -----------------------------------------------
function build_tools_deps() {
  # To run PTF nanomsg tests.
  sudo pip3 install nnpy
  sudo apt-get install -y libnanomsg-dev
}
# ! ------  END P4TOOLS -----------------------------------------------


# Build the dependencies necessary for the P4Tools platform.
if [ "$ENABLE_TEST_TOOLS" == "ON" ]; then
  build_tools_deps
fi

# Build with Clang instead of GCC.
if [ "$COMPILE_WITH_CLANG" == "ON" ]; then
  export CC=clang
  export CXX=clang++
fi

# Strong optimization.
export CXXFLAGS="${CXXFLAGS} -O3"
# Toggle unity compilation.
CMAKE_FLAGS+="-DCMAKE_UNITY_BUILD=${CMAKE_UNITY_BUILD} "
# Toggle static builds.
CMAKE_FLAGS+="-DBUILD_STATIC_RELEASE=${BUILD_STATIC_RELEASE} "
# Toggle the installation of the tools back end.
CMAKE_FLAGS+="-DENABLE_TEST_TOOLS=${ENABLE_TEST_TOOLS} "
# RELEASE should be default, but we want to make sure.
CMAKE_FLAGS+="-DCMAKE_BUILD_TYPE=RELEASE "
# Treat warnings as errors.
CMAKE_FLAGS+="-DENABLE_WERROR=${ENABLE_WERROR} "
# Enable sanitizers.
CMAKE_FLAGS+="-DENABLE_SANITIZERS=${ENABLE_SANITIZERS} "
# Enable auto var initialization with pattern.
CMAKE_FLAGS+="-DBUILD_AUTO_VAR_INIT_PATTERN=${BUILD_AUTO_VAR_INIT_PATTERN} "

if [ "$ENABLE_SANITIZERS" == "ON" ]; then
  CMAKE_FLAGS+="-DENABLE_GC=OFF"
  echo "Warning: building with ASAN and UBSAN sanitizers, GC must be disabled."
fi

# Run CMake in the build folder.
if [ -e build ]; then /bin/rm -rf build; fi
mkdir -p ${P4C_DIR}/build
cd ${P4C_DIR}/build
cmake ${CMAKE_FLAGS} ..

# If CMAKE_ONLY is active, only run CMake. Do not build.
if [ "$CMAKE_ONLY" == "OFF" ]; then
  make
  sudo make install
  # Print ccache statistics after building
  ccache -p -s
fi


if [[ "${IMAGE_TYPE}" == "build" ]] ; then
  cd ~
  sudo apt-get purge -y ${P4C_DEPS} git
  sudo apt-get autoremove --purge -y
  rm -rf ${P4C_DIR} /var/cache/apt/* /var/lib/apt/lists/*
  echo 'Build image ready'

elif [[ "${IMAGE_TYPE}" == "test" ]] ; then
  echo 'Test image ready'

fi
