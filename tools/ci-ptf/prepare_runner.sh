#!/bin/bash -e

# Copyright 2022-present Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Script to prepare runner: install all packages to build and run virtual machine.

apt update
apt install -y \
    bridge-utils \
    qemu-kvm \
    libvirt-daemon-system \
    libvirt-clients \
    virtinst \
    libguestfs-tools \
    wait-for-it \
    whois \
    sshpass

# Re-define default network to always assign the same IP address
virsh net-destroy default
virsh net-undefine default
virsh net-define ./tools/ci-ptf/default_network.xml
virsh net-autostart default
virsh net-start default

# Setup permissions to access repository from VM
USER=$(stat -c '%U' "$(pwd)")
GROUP=$(stat -c '%G' "$(pwd)")
echo "user = \"$USER\"" >> /etc/libvirt/qemu.conf
echo "group = \"$GROUP\"" >> /etc/libvirt/qemu.conf
systemctl restart libvirtd
