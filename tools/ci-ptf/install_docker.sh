#!/bin/bash

apt install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release

mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg

echo \
    "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
    $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null

apt update
apt install -y \
    docker-ce \
    docker-ce-cli \
    containerd.io \
    docker-compose-plugin

# allow sudo-less docker management
groupadd docker
usermod -aG docker ubuntu

# change data root directory outside inner VM due to lack of disk space
mkdir /mnt/docker
mkdir -p /etc/docker
cat << EOJSON > /etc/docker/daemon.json
{
    "data-root": "/mnt/docker"
}
EOJSON
