#!/usr/bin/env bash

sudo apt-get -y install libnuma-dev clang-6.0 libc6-dev-i386
sudo apt-get -y install flex bison
sudo python -m pip install scapy

#P4c dependencies
sudo apt-get -y install cmake g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply tcpdump

# Protobuf
git clone https://github.com/google/protobuf.git
cd protobuf
git checkout v3.2.0
export CFLAGS="-Os"
export CXXFLAGS="-Os"
export LDFLAGS="-Wl,-s"
./autogen.sh
./configure
make clean
make
sudo make install
sudo ldconfig
unset CFLAGS CXXFLAGS LDFLAGS
cd python
sudo python setup.py install
cd ../..


#Install clang-10
cd /vagrant
git clone https://github.com/P4-Research/llvm-project
cd llvm-project
mkdir build
cd build
cmake -DLLVM_TARGETS_TO_BUILD=BPF -DLLVM_ENABLE_PROJECTS=clang -G "Unix Makefiles" ../llvm
make
sudo make install


# Clone ovs
cd /home/vagrant
git clone http://10.254.188.33/diaas/ovs.git
cd ovs/
git checkout bpf-actions
git submodule update --init

# Download & Build DPDK
cd /home/vagrant/
wget http://fast.dpdk.org/rel/dpdk-18.11.2.tar.xz
tar xf dpdk-18.11.2.tar.xz
export DPDK_DIR=/home/vagrant/dpdk-stable-18.11.2
export RTE_SDK=$DPDK_DIR
export RTE_TARGET=x86_64-native-linuxapp-gcc
export DPDK_BUILD=$DPDK_DIR/$RTE_TARGET/
cd $DPDK_DIR
make install T=$RTE_TARGET DESTDIR=install

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

# Build OVS (first time only)
cd /home/vagrant/ovs/
./boot.sh
./configure --with-dpdk=$DPDK_BUILD CFLAGS="-g -O2 -Wno-cast-align"
make -j 2
sudo make install

# Create OVS database files and folders
sudo mkdir -p /usr/local/etc/openvswitch
sudo mkdir -p /usr/local/var/run/openvswitch
cd /home/vagrant/ovs/ovsdb/
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

#Install scapy
git clone https://github.com/secdev/scapy.git
cd scapy
sudo python setup.py install