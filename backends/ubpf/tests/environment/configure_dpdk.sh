#! /bin/sh

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

# This script helps to configure DPDK interfaces and sets up hugepages.
# It has to be done after every machine reboot.

DPDK_DIR=/home/vagrant/dpdk-stable-18.11.2
RTE_SDK=
RTE_TARGET=x86_64-native-linuxapp-gcc
DPDK_BUILD=//
sudo modprobe uio
sudo insmod //kmod/igb_uio.ko
sudo insmod //kmod/rte_kni.ko lo_mode=lo_mode_ring
sudo ifconfig enp0s8 down
sudo /usertools/dpdk-devbind.py -b igb_uio 0000:00:08.0
sudo ifconfig enp0s9 down
sudo /usertools/dpdk-devbind.py -b igb_uio 0000:00:09.0
sudo mkdir -p /mnt/huge
(mount | grep hugetlbfs) > /dev/null || sudo mount -t hugetlbfs nodev /mnt/huge
echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
