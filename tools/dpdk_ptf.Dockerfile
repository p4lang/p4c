FROM ghcr.io/ipdk-io/ipdk-ubuntu2004-x86_64:ipdk_v23.07

# Prep
ENV IPDK_INSTALL_DIR=/root/ipdk_install
ENV PATH="${PATH}:/root/.local/bin"
WORKDIR /root
RUN rm -rf p4*
COPY ./p4c /root/p4c
COPY ./p4sde /root/p4sde
COPY ./.ccache /root/.ccache
RUN apt-get update && apt-get install rsync git ccache libhugetlbfs-bin -y

# Update GCC to 11 -> GCC-9 cause config problem, remove after base OS is updated to 22.04
RUN sudo apt install build-essential manpages-dev software-properties-common -y
RUN sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN sudo apt update && sudo apt install gcc-11 g++-11 -y
RUN sudo update-alternatives --remove-all cpp
RUN sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 --slave /usr/bin/g++ g++ /usr/bin/g++-9 --slave /usr/bin/gcov gcov /usr/bin/gcov-9 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-9 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-9  --slave /usr/bin/cpp cpp /usr/bin/cpp-9 \
    && sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 --slave /usr/bin/g++ g++ /usr/bin/g++-11 --slave /usr/bin/gcov gcov /usr/bin/gcov-11 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11  --slave /usr/bin/cpp cpp /usr/bin/cpp-11;

# p4sde (tdi requires gcc-11)
WORKDIR /root/p4sde
RUN pip3 install distro && python3 ./tools/setup/install_dep.py
RUN ./autogen.sh && ./configure --prefix=$IPDK_INSTALL_DIR \
    && make -j4 && make install

# infrap4d deps
WORKDIR /root
RUN sudo apt install libatomic1 libnl-route-3-dev openssl -y
RUN rsync -avh ./networking-recipe/deps_install/* $IPDK_INSTALL_DIR \
    && rsync -avh ./networking-recipe/install/* $IPDK_INSTALL_DIR

# P4C
ENV CMAKE_UNITY_BUILD="ON"
ENV ENABLE_TEST_TOOLS="ON"
ENV INSTALL_DPDK="ON"   
ENV INSTALL_BMV2="ON"
ENV INSTALL_EBPF="OFF"
ENV IMAGE_TYPE="test"
ENV CTEST_PARALLEL_LEVEL="4"
ENV CMAKE_FLAGS="-DENABLE_P4TC=OFF -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_UBPF=OFF -DENABLE_GTESTS=OFF -DENABLE_P4TEST=OFF -DENABLE_P4C_GRAPHS=OFF -DIPDK_INSTALL_DIR=$IPDK_INSTALL_DIR "
WORKDIR /root/p4c
RUN apt-get install python3-dev -y # Required by packges in ci-build, remove after base OS is updated to 22.04
RUN tools/ci-build.sh 

# Disable github aciton default THP setting
# CMD ["sudo", "hugeadm", "--thp-madvise "]


