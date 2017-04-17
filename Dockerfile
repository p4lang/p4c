FROM p4lang/third-party:stable
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

ENV BM_DEPS automake \
            build-essential \
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
                    libboost-thread1.58.0 \
                    libgmp10 libjudydebian1 \
                    libpcap0.8 \
                    python
COPY . /behavioral-model/
WORKDIR /behavioral-model/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $BM_DEPS $BM_RUNTIME_DEPS && \
    ./autogen.sh && \
    ./configure --enable-debugger --with-pdfixed --with-stress-tests && \
    make && \
    make install-strip && \
    ldconfig && \
    (test "$IMAGE_TYPE" = "build" && \
      apt-get purge -y $BM_DEPS && \
      apt-get autoremove --purge -y && \
      rm -rf /behavioral-model /var/cache/apt/* /var/lib/apt/lists/* && \
      echo 'Build image ready') || \
    (test "$IMAGE_TYPE" = "test" && \
      echo 'Test image ready')
