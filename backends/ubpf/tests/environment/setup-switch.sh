#!/usr/bin/env bash

# Copyright 2019 Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Dependencies
sudo apt-get -y install libnuma-dev clang-6.0 libc6-dev-i386 unzip
sudo apt-get -y install flex bison
sudo python -m pip install scapy

# P4c dependencies
sudo apt-get -y install cmake g++ git automake libtool libgc-dev bison flex libfl-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ply tcpdump

# Protobuf
cd /home/vagrant
if [ ! -d "/home/vagrant/protobuf" ]
then
  git clone https://github.com/google/protobuf.git
  cd protobuf
  git checkout v3.2.0
  export CFLAGS="-Os"
  export CXXFLAGS="-Os"
  export LDFLAGS="-Wl,-s"
  ./autogen.sh
  ./configure
  make -j$(nproc)
  sudo make install
  sudo ldconfig
  unset CFLAGS CXXFLAGS LDFLAGS
  cd python
  sudo python setup.py install
  cd ../..
fi

# Install clang-10
if ! type "/home/vagrant/llvm-project/build/bin/clang" > /dev/null; then
  cd /home/vagrant
  git clone --depth=50 https://github.com/P4-Research/llvm-project
  cd llvm-project
  mkdir build
  cd build
  cmake -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DLLVM_BUILD_TOOLS=Off \
  -DLLVM_BUILD_RUNTIME=Off \
  -DLLVM_INCLUDE_TESTS=Off \
  -DLLVM_INCLUDE_EXAMPLES=Off \
  -DLLVM_ENABLE_BACKTRACES=Off \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_ENABLE_OCAMLDOC=Off \
  -DLLVM_BUILD_UTILS=Off \
  -DLLVM_BUILD_DOCS=Off \
  -DLLVM_TARGETS_TO_BUILD=BPF \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DLLVM_OPTIMIZED_TABLEGEN=On \
  -DCLANG_ENABLE_ARCMT=Off \
  -DCLANG_ENABLE_STATIC_ANALYZER=Off \
  -DCLANG_INCLUDE_TESTS=Off \
  -DCLANG_BUILD_EXAMPLES=Off \
  ../llvm
  make -j$(nproc) clang
  sudo make install
  export CLANG_DIR=$(pwd)
fi

# Clone P4rt-OVS
if [ ! -d "/home/vagrant/p4rt-ovs" ]
then
 cd /home/vagrant
 git clone https://github.com/Orange-OpenSource/p4rt-ovs.git
 cd p4rt-ovs/
 git submodule update --init
fi

# Download & Build DPDK
export DPDK_DIR=/home/vagrant/dpdk-stable-18.11.2
export RTE_SDK=$DPDK_DIR
export RTE_TARGET=x86_64-native-linuxapp-gcc
export DPDK_BUILD=$DPDK_DIR/$RTE_TARGET/

if [ ! -d "/home/vagrant/dpdk-stable-18.11.2" ]
then
 cd /home/vagrant/
 wget http://fast.dpdk.org/rel/dpdk-18.11.2.tar.xz
 tar xf dpdk-18.11.2.tar.xz
 cd $DPDK_DIR
 make install T=$RTE_TARGET DESTDIR=install
fi

# Setup DPDK kernel modules
cd /home/vagrant
sudo modprobe uio
sudo insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko
sudo insmod $RTE_SDK/$RTE_TARGET/kmod/rte_kni.ko "lo_mode=lo_mode_ring"

# Add enp0s8 and enp0s9 interfaces to DPDK
sudo ifconfig enp0s8 down
sudo $RTE_SDK/usertools/dpdk-devbind.py -b igb_uio 0000:00:08.0
sudo ifconfig enp0s9 down
sudo $RTE_SDK/usertools/dpdk-devbind.py -b igb_uio 0000:00:09.0

# To view these interfaces run the following command:
# $RTE_SDK/tools/dpdk_nic_bind.py --status

# Configure Huge Pages
cd /home/vagrant
sudo mkdir -p /mnt/huge
(mount | grep hugetlbfs) > /dev/null || sudo mount -t hugetlbfs nodev /mnt/huge
echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

# Note: you can verify if huge pages are configured properly using the following command:
# grep -i huge /proc/meminfo

# Build OVS
cd /home/vagrant/p4rt-ovs/
./boot.sh
./configure --with-dpdk=$DPDK_BUILD CFLAGS="-g -O2 -Wno-cast-align"
make -j$(nproc)
sudo make install

# Create OVS database files and folders
sudo mkdir -p /usr/local/etc/openvswitch
sudo mkdir -p /usr/local/var/run/openvswitch
cd /home/vagrant/p4rt-ovs/ovsdb/
sudo ./ovsdb-tool create /usr/local/etc/openvswitch/conf.db ../vswitchd/vswitch.ovsschema

sudo ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock --remote=db:Open_vSwitch,Open_vSwitch,manager_options     --pidfile --detach
sudo ovs-vsctl --no-wait init
sudo ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
sudo ovs-vswitchd unix:/usr/local/var/run/openvswitch/db.sock --pidfile --detach --verbose --log-file=/home/vagrant/oko.log
sudo ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev

sudo ovs-vsctl add-port br0 enp0s8 -- set Interface enp0s8 type=dpdk \
            options:dpdk-devargs=0000:00:08.0 \
            ofport_request=1

sudo ovs-vsctl add-port br0 enp0s9 -- set Interface enp0s9 type=dpdk \
            options:dpdk-devargs=0000:00:09.0 \
            ofport_request=2


# Clone and install PTF
if ! type "ptf" > /dev/null; then
 cd /home/vagrant
 git clone https://github.com/p4lang/ptf.git
 cd ptf/
 sudo python setup.py install
fi

# Install nanomsg and nnpy
if [ ! -d "/home/vagrant/nanomsg" ]
then
 cd /home/vagrant
 git clone https://github.com/nanomsg/nanomsg.git
 cd nanomsg
 mkdir build
 cd build
 cmake ..
 cmake --build .
 sudo cmake --build . --target install
 sudo ldconfig

 sudo cp /home/vagrant/nanomsg/build/*.* /usr/lib

 cd /home/vagrant/ptf/ptf_nn
 sudo python -m pip install nnpy
 ./check-nnpy.py
fi

# Install P4c
if [ ! -d "/home/vagrant/p4c" ]
then
 cd /home/vagrant
 git clone https://github.com/p4lang/p4c.git
 cd p4c
 git submodule update --init --recursive
 mkdir build
 cd build
 cmake ..
 make -j$(nproc)
 sudo make install
fi

# Install scapy
if ! type "scapy" > /dev/null; then
 cd /home/vagrant
 git clone https://github.com/secdev/scapy.git
 cd scapy
 sudo python setup.py install
fi
