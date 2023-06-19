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

set -ex

# Dependencies
sudo apt-get -y install libnuma-dev clang-6.0 libc6-dev-i386 python python-pip python-dev libffi-dev tcpdump
sudo apt-get -y install flex bison
sudo apt-get -y install cmake g++ git automake libtool libgc-dev bison flex libfl-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ply tcpdump

# Install scapy
if ! type "scapy" > /dev/null; then
 cd /home/vagrant
 git clone https://github.com/secdev/scapy.git
 cd scapy
 sudo python setup.py install
fi

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
 ctest .
 sudo cmake --build . --target install
 sudo ldconfig

 sudo cp /home/vagrant/nanomsg/build/*.* /usr/lib

 cd /home/vagrant/ptf/ptf_nn
 sudo python -m pip install nnpy
 ./check-nnpy.py
fi

# Clone P4c
if [ ! -d "/home/vagrant/p4c" ]
then
 cd /home/vagrant
 git clone https://github.com/p4lang/p4c.git
 cd p4c
fi

# Set promiscuous mode
sudo ip link set enp0s8 promisc on
sudo sed -i "$(( $( wc -l < /etc/rc.local) -2 ))a ip link set enp0s8 promisc on\n" /etc/rc.local

sudo ip link set enp0s9 promisc on
sudo sed -i "$(( $( wc -l < /etc/rc.local) -2 ))a ip link set enp0s9 promisc on\n" /etc/rc.local
