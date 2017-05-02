FROM p4lang/pi:latest
MAINTAINER Seth Fowler <seth.fowler@barefootnetworks.com>

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build

ENV P4C_DEPS automake \
             bison \
             build-essential \
             flex \
             g++ \
             libboost-dev \
             libboost-iostreams1.58-dev \
             libgc-dev \
             libgmp-dev \
             libtool \
             pkg-config \
             python-ipaddr \
             python-scapy \
             tcpdump
ENV P4C_RUNTIME_DEPS cpp \
                     libboost-iostreams1.58.0 \
                     libgc1c2 \
                     libgmp10 \
                     libgmpxx4ldbl \
                     python
COPY . /p4c/
WORKDIR /p4c/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $P4C_DEPS $P4C_RUNTIME_DEPS && \
    ./bootstrap.sh && \
    cd build && \
    make && \
    make install && \
    (test "$IMAGE_TYPE" = "build" && \
      apt-get purge -y $P4C_DEPS && \
      apt-get autoremove --purge -y && \
      rm -rf /p4c /var/cache/apt/* /var/lib/apt/lists/* && \
      echo 'Build image ready') || \
    (test "$IMAGE_TYPE" = "test" && \
      echo 'Test image ready')
