#!/bin/bash

THIS_DIR=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )
P4C_DIR=$(readlink -f ${THIS_DIR}/..)

# Get distribution info
DISTRO=$(lsb_release -i | awk -F: '{print $2}' | xargs)
VERSION=$(lsb_release -r | awk -F: '{print $2}' | xargs)

# Define supported distributions and versions
SUPPORTED_DISTROS=("Ubuntu" "Pop")
SUPPORTED_UBUNTU_VERSIONS=("20.04" "22.04" "24.04")

# Function to check if an element exists in an array
contains() {
    local element
    for element in "${@:2}"; do
        if [[ "$element" == "$1" ]]; then
            return 0
        fi
    done
    return 1
}

# Check distribution and version
if contains "$DISTRO" "${SUPPORTED_DISTROS[@]}"; then
    if [[ "$DISTRO" == "Ubuntu" ]]; then
        if contains "$VERSION" "${SUPPORTED_UBUNTU_VERSIONS[@]}"; then
            echo "Supported distribution: $DISTRO $VERSION"
            exit 0
        else
            echo "Error: Unsupported Ubuntu version: $VERSION"
            exit 1
        fi
    else
        # Assume all versions of Pop!_OS and Linux Mint are supported
        echo "Supported distribution: $DISTRO $VERSION"
    fi
else
    echo "Error: Unsupported distribution: $DISTRO"
    exit 1
fi

# Whether to install dependencies required to run PTF-ebpf tests
: "${INSTALL_PTF_EBPF_DEPENDENCIES:=OFF}"
# The build generator to use. Defaults to Make.
: "${BUILD_GENERATOR:="ninja"}"
# BMv2 is enable by default.
: "${ENABLE_BMV2:=ON}"
# eBPF is enabled by default.
: "${ENABLE_EBPF:=ON}"
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


. /etc/lsb-release

# In Docker builds, sudo is not available. So make it a noop.
if [ "$IN_DOCKER" == "TRUE" ]; then
  echo "Executing within docker container."
  function sudo() { command "$@"; }
fi


# ! ------  BEGIN CORE -----------------------------------------------
P4C_DEPS="bison \
          flex \
          cmake \
          build-essential \
          ccache \
          ninja-build \
          g++ \
          git \
          lld \
          libboost-graph-dev \
          libboost-iostreams-dev \
          libfl-dev \
          python3 \
          python3-pip \
          python3-setuptools"

sudo apt-get update
sudo apt-get install -y --no-install-recommends ${P4C_DEPS}

sudo apt-get install -y python3-poetry python3-venv
poetry install
poetry shell

if [ "${BUILD_GENERATOR,,}" == "ninja" ] && [ ! $(command -v ninja) ]
then
    echo "Selected ninja as build generator, but ninja could not be found."
    exit 1
fi

# ! ------  END CORE -----------------------------------------------

ccache --set-config max_size=1G

# ! ------  BEGIN BMV2 -----------------------------------------------
function build_bmv2() {
  P4C_RUNTIME_DEPS_BOOST="libboost-graph1.7* libboost-iostreams1.7*"

  P4C_RUNTIME_DEPS="cpp \
                    ${P4C_RUNTIME_DEPS_BOOST} \
                    libgc1* \
                    libgmp-dev \
                    libnanomsg-dev"

   sudo apt-get install -y wget ca-certificates
    # Add the p4lang opensuse repository.
    echo "deb http://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/ /" | sudo tee /etc/apt/sources.list.d/home:p4lang.list
    curl -fsSL https://download.opensuse.org/repositories/home:p4lang/xUbuntu_${DISTRIB_RELEASE}/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
    P4C_RUNTIME_DEPS+=" p4lang-bmv2"

  sudo apt-get update && sudo apt-get install -y --no-install-recommends ${P4C_RUNTIME_DEPS}

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
                 llvm \
                 clang \
                 tcpdump \
                 iproute2 \
                 iptables \
                 net-tools"

  sudo apt-get install -y --no-install-recommends ${P4C_EBPF_DEPS}
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

# ! ------  BEGIN DPDK -----------------------------------------------
function build_dpdk() {
  # Replace existing Protobuf with one that works.
  # TODO: Debug protobuf mismatch.
  sudo -E pip3 uninstall -y protobuf
  sudo pip3 install protobuf==3.20.3 netaddr==0.9.0
}

if [[ "${ENABLE_DPDK}" == "ON" ]]; then
  build_dpdk
fi
# ! ------  END DPDK -----------------------------------------------

# ! ------  BEGIN TOFINO --------------------------------------------

function build_tofino() {
    P4C_TOFINO_PACKAGES="rapidjson-dev"
    sudo apt-get install -y --no-install-recommends ${P4C_TOFINO_PACKAGES}
    sudo pip3 install jsl==0.2.4 pyinstaller==6.11.0
}

if [[ "${ENABLE_TOFINO}" == "ON" ]]; then
  echo "Installing Tofino dependencies"
  build_tofino
fi
# ! ------  END TOFINO ----------------------------------------------

echo "Test image ready"

