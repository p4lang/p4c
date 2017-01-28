FROM p4lang/third-party:latest
MAINTAINER Seth Fowler <seth.fowler@barefootnetworks.com>

# Install dependencies.
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get install -y \
      libjudy-dev \
      libgmp-dev \
      libpcap-dev \
      libboost-program-options-dev \
      libboost-system-dev \
      libboost-filesystem-dev \
      libboost-thread-dev

# Copy entire repository.
COPY . /behavioral-model/
WORKDIR /behavioral-model/

# Build.
RUN ./autogen.sh && \
    ./configure && \
    make && \
    make install && \
    ldconfig

# Clean up to reduce the size of the final image.
WORKDIR /
RUN rm -rf behavioral-model
