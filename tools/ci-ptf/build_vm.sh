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

# Script to build virtual machine disk image.

USER_NAME="ubuntu"
# Password: ubuntu
# Newer version of mkpasswd from Ubuntu 22.04 generates string that is not correctly understand by
# Ubuntu 20.04, so hardcode password hash
# TODO: use option `--password` from virt-builder command instead, e.g. `--password ubuntu:password:ubuntu`
USER_PASS="Q7KzRGQd70ZS6"
USER_HOME=$(pwd)

function create_kernel_packages_str() {
	KERNEL_PACKAGES=""
	while (( "$#" )); do
		if [ "x$KERNEL_PACKAGES" != "x" ]; then
			KERNEL_PACKAGES+=","
		fi
		KERNEL_PACKAGES+="linux-image-$1-generic,linux-modules-$1-generic,linux-modules-extra-$1-generic"
		shift
	done
}

# $@ (list of kernels versions) must be passed without quotes to allow split into separate arguments
create_kernel_packages_str $@

# Note: Image with Ubuntu 20.04 is preinstalled on logical volume which virt-builder is unable to resize
virt-builder "$OS_TYPE" \
    --memsize 2048 \
    --hostname "$VM_NAME" \
    --network \
    --timezone "$(cat /etc/timezone)" \
    --format qcow2 -o "$DISK_IMAGE" \
    --update \
    --install "$KERNEL_PACKAGES" \
    --run-command "useradd -p $USER_PASS -s /bin/bash -m -d $USER_HOME -G sudo $USER_NAME" \
    --edit '/etc/sudoers:s/^%sudo.*/%sudo	ALL=(ALL) NOPASSWD:ALL/' \
    --edit '/etc/default/grub:s/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="console=tty0 console=ttyS0,115200n8"/' \
    --run-command update-grub \
    --upload ./tools/ci-ptf/netcfg.yaml:/etc/netplan/netcfg.yaml \
    --run-command "chown root:root /etc/netplan/netcfg.yaml" \
    --append-line "/etc/fstab:runner $USER_HOME 9p defaults,_netdev 0 0" \
    --append-line "/etc/fstab:/dev/vdb1 /mnt/docker ext4 defaults 0 0" \
    --run ./tools/ci-ptf/install_docker.sh \
    --firstboot-command "dpkg-reconfigure openssh-server"

# Enable compression on disk
mv "$DISK_IMAGE" "$DISK_IMAGE.old"
qemu-img convert -O qcow2 -c "$DISK_IMAGE.old" "$DISK_IMAGE"
rm -f "$DISK_IMAGE.old"
