FROM ubuntu:14.04
MAINTAINER Seth Fowler <seth.fowler@barefootnetworks.com>

# Install dependencies.
RUN apt-get update && \
    apt-get install -y \
      automake \
      bison \
      build-essential \
      flex \
      g++ \
      git \
      libboost-dev \
      libgc-dev \
      libgmp-dev \
      libtool \
      python2.7 \
      python-ipaddr \
      python-scapy \
      tcpdump

# Copy entire repository.
COPY . /p4c/
WORKDIR /p4c/

# Build.
RUN ./bootstrap.sh && \
    cd build && \
    make -j8
