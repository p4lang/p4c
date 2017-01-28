FROM ubuntu:14.04
MAINTAINER Seth Fowler <seth.fowler@barefootnetworks.com>

# Install dependencies.
ENV DEBIAN_FRONTEND noninteractive
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

# Default to using 8 make jobs, which is appropriate for local builds. On CI
# infrastructure it will often be best to override this.
ARG MAKEFLAGS
ENV MAKEFLAGS ${MAKEFLAGS:-j8}

# Clone and build behavioral-model, which is needed by some tests.
ENV BEHAVIORAL_MODEL_REPO https://github.com/sethfowler/behavioral-model.git
ENV BEHAVIORAL_MODEL_TAG 4f85ec95da24
RUN git clone --recursive $BEHAVIORAL_MODEL_REPO /behavioral-model && \
    cd /behavioral-model && \
    git reset --hard $BEHAVIORAL_MODEL_TAG && \
    ./install_deps.sh && \
    ./autogen.sh && \
    ./configure && \
    make && \
    make install && \
    ldconfig

# Copy entire repository.
COPY . /p4c/
WORKDIR /p4c/

# Build.
RUN ./bootstrap.sh && \
    cd build && \
    make
