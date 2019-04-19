FROM p4lang/behavioral-model:latest

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build

ENV P4C_DEPS bison \
             build-essential \
             cmake \
             flex \
             g++ \
             libboost-dev \
             libboost-graph-dev \
             libboost-iostreams1.58-dev \
             libfl-dev \
             libgc-dev \
             libgmp-dev \
             pkg-config \
             python-ipaddr \
             python-pip \
             python-setuptools \
             tcpdump
ENV P4C_EBPF_DEPS libpcap-dev \
             libelf-dev \
             llvm \
             clang \
             iproute2 \
             net-tools
ENV P4C_RUNTIME_DEPS cpp \
                     libboost-graph1.58.0 \
                     libboost-iostreams1.58.0 \
                     libgc1c2 \
                     libgmp10 \
                     libgmpxx4ldbl \
                     python
ENV P4C_PIP_PACKAGES tenjin \
                     pyroute2 \
                     ply==3.8 \
                     scapy==2.4.0
COPY . /p4c/
WORKDIR /p4c/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $P4C_DEPS $P4C_EBPF_DEPS $P4C_RUNTIME_DEPS && \
    pip install $P4C_PIP_PACKAGES && \
    mkdir build && \
    cd build && \
    cmake .. '-DCMAKE_CXX_FLAGS:STRING=-O3' && \
    make && \
    make install && \
    /usr/local/bin/ccache -p -s && \
    ( \
      (test "$IMAGE_TYPE" = "build" && \
        apt-get purge -y $P4C_DEPS && \
        apt-get autoremove --purge -y && \
        rm -rf /p4c /var/cache/apt/* /var/lib/apt/lists/* && \
        echo 'Build image ready') || \
      (test "$IMAGE_TYPE" = "test" && \
        echo 'Test image ready') \
    )
