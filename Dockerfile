FROM p4lang/behavioral-model:latest
MAINTAINER Seth Fowler <seth.fowler@barefootnetworks.com>

ENV P4C_DEPS automake \
             bison \
             build-essential \
             flex \
             g++ \
             libboost-dev \
             libgc-dev \
             libgmp-dev \
             libtool \
             pkg-config \
             python \
             python-ipaddr \
             python-scapy \
             tcpdump
COPY . /p4c/
WORKDIR /p4c/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $P4C_DEPS && \
    ./bootstrap.sh && \
    cd build && \
    make
