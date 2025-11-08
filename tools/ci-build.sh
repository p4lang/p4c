#!/bin/bash

# Script for building P4C for continuous integration builds.

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
# all libraries except for glibc statically.
: "${STATIC_BUILD_WITH_DYNAMIC_GLIBC:=OFF}"
# This creates a release build that includes link time optimization and links
# all libraries except for glibc and libstdc++ statically.
: "${STATIC_BUILD_WITH_DYNAMIC_STDLIB:=OFF}"
# No questions asked during package installation.
: "${DEBIAN_FRONTEND:=noninteractive}"
# Whether to install dependencies required to run PTF-ebpf tests
: "${INSTALL_PTF_EBPF_DEPENDENCIES:=OFF}"
# Whether to build and run GTest unit tests.
: "${ENABLE_GTESTS:=ON}"
# Whether to treat warnings as errors.
: "${ENABLE_WERROR:=ON}"
# Compile with Clang compiler
: "${COMPILE_WITH_CLANG:=OFF}"
# Compile with sanitizers (UBSan, ASan)
: "${ENABLE_SANITIZERS:=OFF}"
# Only execute the steps necessary to successfully run CMake.
: "${CMAKE_ONLY:=OFF}"
# The build generator to use. Defaults to Make.
: "${BUILD_GENERATOR:="Unix Makefiles"}"
# Build with -ftrivial-auto-var-init=pattern to catch more bugs caused by
# uninitialized variables.
: "${BUILD_AUTO_VAR_INIT_PATTERN:=OFF}"
# BMv2 is enable by default.
: "${ENABLE_BMV2:=ON}"
# eBPF is enabled by default.
: "${ENABLE_EBPF:=ON}"
# P4TC is enabled by default.
: "${ENABLE_P4TC:=ON}"
# P4TC STF is only enabled when running p4tc tagged PRs.
: "${INSTALL_STF_P4TC_DEPENDENCIES:=OFF}"
# This is the list of back ends that can be enabled.
# Back ends can be enabled from the command line with "ENABLE_[backend]=TRUE/FALSE"
ENABLE_BACKENDS=("TOFINO" "BMV2" "EBPF" "UBPF" "DPDK"
                 "P4TC" "P4FMT" "P4TEST" "P4C_GRAPHS"
                 "TEST_TOOLS"
)
function build_cmake_enabled_backend_string() {
  CMAKE_ENABLE_BACKENDS=""
  for backend in "${ENABLE_BACKENDS[@]}";
  do
    enable_var=ENABLE_${backend}
    if [ -n "${!enable_var}" ]; then
      echo "${enable_var}=${!enable_var} is set."
      CMAKE_ENABLE_BACKENDS+="-D${enable_var}=${!enable_var} "
    fi
  done
}

pushd ${P4C_DIR}

. /etc/lsb-release

# In Docker builds, sudo is not available. So make it a noop.
if [ "$IN_DOCKER" = "TRUE" ]; then
  echo "Executing within docker container."
  # No-op function; just execute the command
  sudo() { "$@"; }
else
  # Preserve PATH and environment variables when using sudo
  sudo() { command sudo -E env PATH="$PATH" "$@"; }
fi


# ! ------  BEGIN CORE -----------------------------------------------
P4C_DEPS="bison \
          build-essential \
          ccache \
          flex \
          ninja-build \
          g++ \
          git \
          lld \
          libboost-dev \
          libboost-graph-dev \
          libboost-iostreams-dev \
          libfl-dev \
          pkg-config \
          python3 \
          python3-pip \
          python3-setuptools \
          tcpdump"

# TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" != "18.04" ]] ; then
  P4C_DEPS+=" cmake"
fi

sudo apt-get update
sudo apt-get install -y --no-install-recommends ${P4C_DEPS}
# Set up uv for Python dependency management.
# TODO: Consider using a system-provided package here.
sudo apt-get install -y python3-venv curl
curl -LsSf https://astral.sh/uv/0.6.12/install.sh | sh
uv sync
uv tool update-shell

if [ "${BUILD_GENERATOR,,}" == "ninja" ] && [ ! $(command -v ninja) ]
then
    echo "Selected ninja as build generator, but ninja could not be found."
    exit 1
fi

# ! ------  END CORE -----------------------------------------------

  # TODO: Remove this check once 18.04 is deprecated.
if [[ "${DISTRIB_RELEASE}" == "18.04" ]] ; then
  ccache --set-config cache_dir=.ccache
  # For Ubuntu 18.04 install the pypi-supplied version of cmake instead.
  uv pip install cmake==3.16.3
fi
ccache --set-config max_size=1G

# ! ------  BEGIN BMV2 -----------------------------------------------
function build_bmv2() {

  P4C_RUNTIME_DEPS="cpp \
                    ${P4C_RUNTIME_DEPS_BOOST} \
                    libgc1* \
                    libgmp-dev \
                    python3-dev \
                    libnanomsg-dev"

  # TODO: Remove this check once 18.04 is deprecated.
  if [[ "${DISTRIB_RELEASE}" == "18.04" ]] ; then
    P4C_RUNTIME_DEPS+=" libboost-graph1.65.1 libboost-iostreams1.65.1 "
  fi

  # TODO: Remove this check once 18.04 is deprecated.
  if [[ "${DISTRIB_RELEASE}" == "18.04" ]] || [[ "$(which simple_switch 2> /dev/null)" != "" ]] ; then
    # Use GCC 9 from https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test
    sudo apt-get update && sudo apt-get install -y software-properties-common
    sudo add-apt-repository -uy ppa:ubuntu-toolchain-r/test
    P4C_RUNTIME_DEPS+=" gcc-9 g++-9"
    export CC=gcc-9
    export CXX=g++-9
  elif [[ "${DISTRIB_RELEASE}" != "24.04" ]] ; then
    echo "Temporarily disabling retrieval of p4lang-bmv2 package until it is working"
    #sudo apt-get install -y wget ca-certificates
    ## Add the p4lang opensuse repository.
    #echo "deb http://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/ /" | sudo tee /etc/apt/sources.list.d/home:p4lang.list
    #curl -fsSL https://download.opensuse.org/repositories/home:p4lang/xUbuntu_${DISTRIB_RELEASE}/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
    #P4C_RUNTIME_DEPS+=" p4lang-bmv2"
  fi

  sudo apt-get update && sudo apt-get install -y --no-install-recommends ${P4C_RUNTIME_DEPS}

  if [[ "${DISTRIB_RELEASE}" != "18.04" ]] ; then
    # To run PTF nanomsg tests. Not available on 18.04.
    uv pip install nnpy==1.4.2
  fi
}

if [[ "${ENABLE_BMV2}" == "ON" ]] ; then
  build_bmv2
fi
# ! ------  END BMV2 -----------------------------------------------

# ! ------  BEGIN EBPF -----------------------------------------------
function build_ebpf() {
  P4C_EBPF_DEPS="libpcap-dev \
                 libelf-dev \
                 zlib1g-dev \
                 iproute2 \
                 iptables \
                 net-tools"
  P4C_EBPF_DEPS+=" llvm clang "
  sudo apt-get install -y --no-install-recommends ${P4C_EBPF_DEPS}
  uv pip install scapy==2.5.0
}

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
    cmake -DCMAKE_BUILD_TYPE=Release -G "${BUILD_GENERATOR}" ..
    cmake --build . -- -j $(nproc)
    sudo cmake --install .

    # install bpftool
    git clone --recurse-submodules --branch v7.3.0 https://github.com/libbpf/bpftool.git /tmp/bpftool
    cd /tmp/bpftool/src
    make "-j$(nproc)"
    sudo make install
    popd
)

if [[ "${ENABLE_EBPF}" == "ON" ]] ; then
  build_ebpf
  if [[ "${INSTALL_PTF_EBPF_DEPENDENCIES}" == "ON" ]] ; then
    install_ptf_ebpf_test_deps
  fi
fi
# ! ------  END EBPF -----------------------------------------------

# ! ------  BEGIN P4TC -----------------------------------------------
function install_stf_p4tc_test_deps() (
    P4C_STF_P4TC_PACKAGES=" libmnl-dev \
                             bridge-utils \
                             python3-venv \
                             virtme-ng \
                             qemu-kvm \
                             clang-19 \
                             python3-scapy \
                             qemu-system-x86"
    sudo apt-get install -y --no-install-recommends ${P4C_STF_P4TC_PACKAGES}
    git clone https://github.com/p4tc-dev/iproute2-p4tc-pub -b master-v17-rc8 ${P4C_DIR}/backends/tc/runtime/iproute2-p4tc-pub
    ${P4C_DIR}/backends/tc/runtime/build-iproute2 ${P4C_DIR}/backends/tc/runtime
    sudo apt-get install -y virtme-ng
)

function build_p4tc() {
  P4TC_DEPS="libpcap-dev \
             libelf-dev \
             zlib1g-dev \
             gcc-multilib \
             net-tools \
             flex \
             libelf-dev \
             pkg-config \
             xtables-addons-source \
             python3 \
             python3-pip \
             wget \
             python3-argcomplete"

  sudo apt-get install -y --no-install-recommends ${P4TC_DEPS}

if [[ "${DISTRIB_RELEASE}" != "24.04" ]] ; then
  wget https://apt.llvm.org/llvm.sh
  sudo chmod +x llvm.sh
  sudo ./llvm.sh 15
  rm llvm.sh
fi

  git clone https://github.com/libbpf/libbpf/ -b v1.5.0 ${P4C_DIR}/backends/tc/runtime/libbpf
  ${P4C_DIR}/backends/tc/runtime/build-libbpf

  if [[ "${INSTALL_STF_P4TC_DEPENDENCIES}" == "ON" ]] ; then
             install_stf_p4tc_test_deps
  fi
}
if [[ "${ENABLE_P4TC}" == "ON" ]] ; then
  build_p4tc
fi
# ! ------  END P4TC -----------------------------------------------

# ! ------  BEGIN DPDK -----------------------------------------------
function build_dpdk() {
  # Replace existing Protobuf with one that works.
  # TODO: Debug protobuf mismatch.
  uv pip install protobuf==3.20.3 netaddr==0.9.0
}

if [[ "${ENABLE_DPDK}" == "ON" ]]; then
  build_dpdk
fi
# ! ------  END DPDK -----------------------------------------------

# ! ------  BEGIN TOFINO --------------------------------------------

function build_tofino() {
    P4C_TOFINO_PACKAGES="rapidjson-dev"
    sudo apt-get install -y --no-install-recommends ${P4C_TOFINO_PACKAGES}
    uv pip install jsl=="0.2.4" pyinstaller=="6.11.0" jsonschema=="4.23.0" pyyaml=="6.0.2"
}

if [[ "${ENABLE_TOFINO}" == "ON" ]]; then
  echo "Installing Tofino dependencies"
  build_tofino
fi
# ! ------  END TOFINO ----------------------------------------------

# ! ------  BEGIN VALIDATION -----------------------------------------------
function build_gauntlet() {
  # Symlink the toz3 extension for the p4 compiler.
  mkdir -p ${P4C_DIR}/extensions
  git clone -b stable https://github.com/p4gauntlet/toz3 extensions/toz3
  # Disable failures on crashes
  CMAKE_FLAGS+="-DVALIDATION_IGNORE_CRASHES=ON "
}

# These steps are necessary to validate the correct compilation of the P4C test
# suite programs. See also https://github.com/p4gauntlet/gauntlet.
if [ "$VALIDATION" == "ON" ]; then
  build_gauntlet
fi
# ! ------  END VALIDATION -----------------------------------------------

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
CMAKE_FLAGS+="-DSTATIC_BUILD_WITH_DYNAMIC_GLIBC=${STATIC_BUILD_WITH_DYNAMIC_GLIBC} "
CMAKE_FLAGS+="-DSTATIC_BUILD_WITH_DYNAMIC_STDLIB=${STATIC_BUILD_WITH_DYNAMIC_STDLIB} "
# Enable GTest.
CMAKE_FLAGS+="-DENABLE_GTESTS=${ENABLE_GTESTS} "
# Release should be default, but we want to make sure.
CMAKE_FLAGS+="-DCMAKE_BUILD_TYPE=Release "
# Treat warnings as errors.
CMAKE_FLAGS+="-DENABLE_WERROR=${ENABLE_WERROR} "
# Enable sanitizers.
CMAKE_FLAGS+="-DENABLE_SANITIZERS=${ENABLE_SANITIZERS} "
# Enable auto var initialization with pattern.
CMAKE_FLAGS+="-DBUILD_AUTO_VAR_INIT_PATTERN=${BUILD_AUTO_VAR_INIT_PATTERN} "
# Assemble the enabled back ends as a single CMake variable.
build_cmake_enabled_backend_string
CMAKE_FLAGS+="${CMAKE_ENABLE_BACKENDS} "

if [ "$ENABLE_SANITIZERS" == "ON" ]; then
  CMAKE_FLAGS+="-DENABLE_GC=OFF"
  echo "Warning: building with ASAN and UBSAN sanitizers, GC must be disabled."
fi

# Run CMake in the build folder.
mkdir -p ${P4C_DIR}/build
uv run cmake -B ${P4C_DIR}/build ${CMAKE_FLAGS} -G "${BUILD_GENERATOR}" .

# If CMAKE_ONLY is active, only run CMake. Do not build.
if [ "$CMAKE_ONLY" == "OFF" ]; then
  uv run cmake --build ${P4C_DIR}/build -- -j $(nproc)
  sudo uv run cmake --install ${P4C_DIR}/build
  # Print ccache statistics after building
  ccache -p -s
fi

if [[ "${IMAGE_TYPE}" == "build" ]] ; then
  sudo apt-get purge -y ${P4C_DEPS} git
  sudo apt-get autoremove --purge -y
  rm -rf ${P4C_DIR} /var/cache/apt/* /var/lib/apt/lists/*
  echo 'Build image ready'

elif [[ "${IMAGE_TYPE}" == "test" ]] ; then
  echo 'Test image ready'

fi

popd
