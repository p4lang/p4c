ARG PARENT_VERSION=latest
FROM p4lang/pi:${PARENT_VERSION}
LABEL maintainer="Antonin Bas <antonin@barefootnetworks.com>"

# Default to using 2 make jobs, which is a good default for CI. If you're
# building locally or you know there are more cores available, you may want to
# override this.
ARG MAKEFLAGS=-j2

# Select the type of image we're building. Use `build` for a normal build, which
# is optimized for image size. Use `test` if this image will be used for
# testing; in this case, the source code and build-only dependencies will not be
# removed from the image.
ARG IMAGE_TYPE=build

ARG CC=gcc
ARG CXX=g++
ARG GCOV=
ARG sswitch_grpc=yes

ENV BM_DEPS automake \
            build-essential \
            clang-3.8 \
            clang-6.0 \
            curl \
            g++-7 \
            git \
            lcov \
            libjudy-dev \
            libgmp-dev \
            libpcap-dev \
            libboost-dev \
            libboost-program-options-dev \
            libboost-system-dev \
            libboost-filesystem-dev \
            libboost-thread-dev \
            libtool \
            pkg-config
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
    ln -sf /usr/bin/python3 /usr/bin/python && \
    ./autogen.sh && \
    if [ "$GCOV" != "" ]; then ./configure --with-pdfixed --with-pi --with-stress-tests --enable-debugger --enable-coverage --enable-Werror; fi && \
    if [ "$GCOV" = "" ]; then ./configure --with-pdfixed --with-pi --with-stress-tests --enable-debugger --enable-Werror; fi && \
    make && \
    make install-strip && \
    (test "$sswitch_grpc" = "yes" && \
      cd targets/simple_switch_grpc/ && \
      ./autogen.sh && \
      ./configure --enable-Werror && \
      make && \
      make install-strip && \
      cd -) || \
    (test "$sswitch_grpc" = "no") && \
    ldconfig && \
    (test "$IMAGE_TYPE" = "build" && \
      apt-get purge -y $BM_DEPS && \
      apt-get autoremove --purge -y && \
      rm -rf /behavioral-model /var/cache/apt/* /var/lib/apt/lists/* && \
      echo 'Build image ready') || \
    (test "$IMAGE_TYPE" = "test" && \
      echo 'Test image ready')
