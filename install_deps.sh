#!/bin/sh
set -e
sudo apt-get install -y \
    automake \
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
    libssl-dev \
    mktemp \
    libffi-dev \
    python-dev \
    python-pip

tmpdir=`mktemp -d -p .`
cd $tmpdir

bash ../build/travis/install-thrift.sh
bash ../build/travis/install-nanomsg.sh
sudo ldconfig
bash ../build/travis/install-nnpy.sh

cd ..
sudo rm -rf $tmpdir
