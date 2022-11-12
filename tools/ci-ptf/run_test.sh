#!/bin/bash

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

# Script to start command under specified kernel

# Translate OS_TYPE into OS_VARIANT which is required for virt-install, see commands
# `virt-install --osinfo list` or `osinfo-query os` for more information and valid values
OS_VARIANT=""
case "$OS_TYPE" in
  "ubuntu-20.04") OS_VARIANT="ubuntu20.04" ;;
  *)
    echo "Unsupported OS_TYPE=$OS_TYPE; please add it to script $0"
    exit 1
    ;;
esac

IP="192.168.122.2"

KERNEL_VERSION="$1"
shift

rm ~/.ssh/known_hosts

set -e

virt-install --import \
    --transient \
    --name "$VM_NAME" \
    --vcpu 2 \
    --ram 3072 \
    --disk path="$WORKING_DISK_IMAGE",format=qcow2 \
    --disk path="$DOCKER_VOLUME_IMAGE",format=qcow2 \
    --os-variant "$OS_VARIANT" \
    --network network:default \
    --graphics none \
    --console pty,target_type=serial \
    --noautoconsole \
    --filesystem "$(pwd)",runner \
    --boot kernel=/tmp/vm/vmlinuz-$KERNEL_VERSION-generic,initrd=/tmp/vm/initrd.img-$KERNEL_VERSION-generic,kernel_args="ro console=tty0 console=ttyS0,115200n8 root=/dev/vda5"

wait-for-it "$IP:22" -t 300 -s -- echo VM ready
sshpass -p ubuntu ssh -o "StrictHostKeyChecking=no" "ubuntu@$IP" uname -a

# we have to cleanup after tests
set +e
sshpass -p ubuntu ssh "ubuntu@$IP" "$@"
exit_code=$?

virsh shutdown "$VM_NAME"
# wait for guest to shutdown itself
while virsh domstate "$VM_NAME" 2>/dev/null; do
    sleep 5
done

exit $exit_code
