FROM p4lang/third-party:stable
LABEL maintainer="Antonin Bas <antonin@barefootnetworks.com>"
LABEL description="This Docker image does not have any dependency on PI or P4 \
Runtime, it only builds bmv2 simple_switch."

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

ENV BM_DEPS automake \
            build-essential \
            curl \
            git \
            libjudy-dev \
            libgmp-dev \
            libpcap-dev \
            libboost-dev \
            libboost-program-options-dev \
            libboost-system-dev \
            libboost-filesystem-dev \
            libboost-thread-dev \
            libtool
ENV BM_RUNTIME_DEPS libboost-program-options1.58.0 \
                    libboost-system1.58.0 \
                    libboost-filesystem1.58.0 \
                    libboost-thread1.58.0 \
                    libgmp10 libjudydebian1 \
                    libpcap0.8 \
                    python
COPY . /behavioral-model/
WORKDIR /behavioral-model/
RUN apt-get update && \
    apt-get install -y --no-install-recommends software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y --no-install-recommends $BM_DEPS $BM_RUNTIME_DEPS && \
    ./autogen.sh && \
    ./configure --enable-debugger && \
    make && \
    make install-strip && \
    ldconfig && \
    apt-get purge -y $BM_DEPS && \
    apt-get autoremove --purge -y && \
    rm -rf /behavioral-model /var/cache/apt/* /var/lib/apt/lists/* && \
    echo 'Build image ready'
