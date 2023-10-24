FROM ghcr.io/ipdk-io/ipdk-ubuntu2004-x86_64:ipdk_v23.07

# Prep
ENV IPDK_INSTALL_DIR=/root/ipdk_install
ENV PATH="${PATH}:/root/.local/bin"
WORKDIR /root
RUN apt-get update
RUN apt-get install -y git
RUN apt-get install rsync -y
RUN rm -rf p4*
RUN git clone --recursive https://github.com/p4lang/p4c.git p4c
RUN git clone --recursive https://github.com/p4lang/p4-dpdk-target.git p4sde
RUN sudo apt-get install libhugetlbfs-bin
RUN sudo hugeadm --thp-madvise 

# Update GCC to 11 -> ubuntu22.04 does not need this
RUN sudo apt install build-essential manpages-dev software-properties-common -y
RUN sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN sudo apt update && sudo apt install gcc-11 g++-11 -y
RUN sudo update-alternatives --remove-all cpp
RUN sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 --slave /usr/bin/g++ g++ /usr/bin/g++-9 --slave /usr/bin/gcov gcov /usr/bin/gcov-9 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-9 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-9  --slave /usr/bin/cpp cpp /usr/bin/cpp-9 \
    && sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 --slave /usr/bin/g++ g++ /usr/bin/g++-11 --slave /usr/bin/gcov gcov /usr/bin/gcov-11 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-11 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-11  --slave /usr/bin/cpp cpp /usr/bin/cpp-11;

# p4sde (tdi requires gcc-11)
WORKDIR /root/p4sde
RUN pip3 install distro
RUN python3 ./tools/setup/install_dep.py
RUN ./autogen.sh
RUN ./configure --prefix=$IPDK_INSTALL_DIR 
RUN make -j4
RUN make install

# infrap4d deps
WORKDIR /root
RUN sudo apt install libatomic1 libnl-route-3-dev openssl -y
RUN rsync -avh ./networking-recipe/deps_install/* $IPDK_INSTALL_DIR 
RUN rsync -avh ./networking-recipe/install/* $IPDK_INSTALL_DIR

# P4C
ENV CMAKE_FLAGS="-DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_UBPF=OFF -DENABLE_GTESTS=OFF -DENABLE_P4TEST=OFF -DENABLE_P4TC=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_TEST_TOOLS=ON -DIPDK_INSTALL_DIR=/root/ipdk_install -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_UBPF=OFF -DENABLE_GTESTS=OFF -DENABLE_P4TEST=OFF -DENABLE_P4TC=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_TEST_TOOLS=ON -DIPDK_INSTALL_DIR=/root/ipdk_install "
WORKDIR /root/p4c
#tools/ci-build.sh # It cannot be used directly since 20.04 is not supported by it, manually install everything then
RUN sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex \
    libfl-dev libboost-dev libboost-iostreams-dev \
    libboost-graph-dev llvm pkg-config python3 python3-pip \
    tcpdump -y
RUN pip3 install --user -r requirements.txt
RUN pip install protobuf==3.20.* 
RUN mkdir build
WORKDIR /root/p4c/build
RUN cmake .. $CMAKE_FLAGS
RUN make -j4

