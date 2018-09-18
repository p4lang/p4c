#!/bin/bash
set -e
ubuntu_release=`lsb_release -s -r`
if [[ "${ubuntu_release}" > "18" ]]
then
    # This older package libssl1.0-dev enables compiling Thrift 0.9.2
    # on Ubuntu 18.04.  Package libssl-dev exists, but Thrift 0.9.2
    # fails to compile when it is installed.
    LIBSSL_DEV="libssl1.0-dev"
else
    LIBSSL_DEV="libssl-dev"
fi
sudo apt-get install -y \
    automake \
    cmake \
    libjudy-dev \
    libgmp-dev \
    libpcap-dev \
    libboost-dev \
    libboost-test-dev \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libevent-dev \
    libtool \
    flex \
    bison \
    pkg-config \
    g++ \
    $LIBSSL_DEV \
    libffi-dev \
    python-dev \
    python-pip \
    wget

tmpdir=`mktemp -d -p .`
cd $tmpdir

bash ../travis/install-thrift.sh
bash ../travis/install-nanomsg.sh
sudo ldconfig
bash ../travis/install-nnpy.sh

cd ..
sudo rm -rf $tmpdir
