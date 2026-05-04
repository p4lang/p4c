#!/bin/bash -e

# SPDX-FileCopyrightText: 2022 Orange
# Copyright 2022-present Orange
#
# SPDX-License-Identifier: Apache-2.0

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
