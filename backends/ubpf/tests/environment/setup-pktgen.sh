#!/usr/bin/env bash

set -ex

sudo apt-get -y install libnuma-dev clang-6.0 libc6-dev-i386 python python-pip python-dev libffi-dev tcpdump
sudo apt-get -y install flex bison
sudo apt-get -y install cmake g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply tcpdump
sudo python -m pip install scapy

# Clone PTF
cd /home/vagrant
git clone https://github.com/p4lang/ptf.git
cd ptf/
sudo python setup.py install

#Install nanomsg
cd /home/vagrant
git clone https://github.com/nanomsg/nanomsg.git
cd nanomsg
mkdir build
cd build
cmake ..
cmake --build .
ctest .
sudo cmake --build . --target install
sudo ldconfig

sudo cp /home/vagrant/nanomsg/build/*.* /usr/lib

cd /home/vagrant/ptf/ptf_nn
sudo python -m pip install nnpy
./check-nnpy.py

cd /home/vagrant
git clone https://github.com/P4-Research/p4c.git
cd p4c
git checkout origin/tests

# Promiscuous mode
sudo ip link set enp0s8 promisc on
sudo sed -i "$(( $( wc -l < /etc/rc.local) -2 ))a ip link set enp0s8 promisc on\n" /etc/rc.local

sudo ip link set enp0s9 promisc on
sudo sed -i "$(( $( wc -l < /etc/rc.local) -2 ))a ip link set enp0s9 promisc on\n" /etc/rc.local