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

# Script to install virtual machine.

if [ -e "$WORKING_DISK_IMAGE" ]; then
  echo "VM already exist - second invocation of $0, so exiting with an error"
  exit 1
fi

# Create storage for docker image(s) in VM
qemu-img create -f qcow2 "$DOCKER_VOLUME_IMAGE" 12G
virt-format --format=qcow2 --filesystem=ext4 -a "$DOCKER_VOLUME_IMAGE"

# Copy boot images for every kernel
mkdir -p /mnt/inner /tmp/vm
guestmount -a "$DISK_IMAGE" -i --ro /mnt/inner/
for version in $KERNEL_VERSIONS; do
  cp "/mnt/inner/boot/initrd.img-$version-generic" /tmp/vm/
  cp "/mnt/inner/boot/vmlinuz-$version-generic" /tmp/vm/
done
guestunmount /mnt/inner

# Make working copy of disk image
cp "$DISK_IMAGE" "$WORKING_DISK_IMAGE"

# Move docker test image into VM, require to boot VM with *any* kernel version
docker save -o p4c.img p4c
chmod +r p4c.img
docker rmi -f p4c
ANY_KERNEL_VERSION="$(echo "$KERNEL_VERSIONS" | awk '{print $1}')"
./tools/ci-ptf/run_test.sh "$ANY_KERNEL_VERSION" docker load -i p4c.img
rm -f p4c.img
