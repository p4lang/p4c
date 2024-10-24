#! /bin/bash

set -e

topdir=$(readlink -f $(dirname $0)/..)
builddir=build.release
# set -x
if [[ $(uname -s) == 'Linux' ]]; then
    parallel_make=`cat /proc/cpuinfo | grep processor | wc -l`
    enable_static="-DENABLE_STATIC_LIBS=ON"
else
    parallel_make=8
    enable_static=""
fi

install_prefix=/usr/local
barefoot_internal="-DENABLE_BAREFOOT_INTERNAL=OFF"
enable_cb="-DENABLE_CLOUDBREAK=OFF"
enable_fr="-DENABLE_FLATROCK=OFF"
pgo=false
lto=false

build_p4c() {
    ${topdir}/bootstrap_bfn_compilers.sh --build-dir $builddir \
                                --p4c-cpp-flags "$*" \
                                -DCMAKE_BUILD_TYPE=RELEASE \
                                -DCMAKE_INSTALL_PREFIX=$install_prefix \
                                -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_UBPF=OFF \
                                -DENABLE_P4TEST=OFF -DENABLE_P4C_GRAPHS=OFF \
                                -DENABLE_DOXYGEN=OFF \
                                $enable_cb $enable_fr \
                                $enable_static \
                                $barefoot_internal \
                                -DENABLE_GTESTS=OFF \
                                -DENABLE_WERROR=OFF \
                                -DENABLE_ASSERTIONS=OFF \
                                -DENABLE_LTO=$lto

    (cd $builddir ; make -j $parallel_make)
}

install_cmake() {
  wget https://cmake.org/files/v3.16/cmake-3.16.3-Linux-x86_64.tar.gz -q -O cmake.tar.gz
  mkdir -p /bfn/cmake
  tar zxf cmake.tar.gz --strip-components=1 -C /bfn/cmake
  rm cmake.tar.gz
}

usage() {
    echo $1
    echo "Usage: ./scripts/package_p4c_for_tofino.sh <optional arguments>"
    echo "   --build-dir <builddir>"
    echo "   --install-prefix <install_prefix>"
    echo "   --barefoot-internal"
    echo "   --enable-cb"
    echo "   --enable-fr"
    echo "   --enable-pgo"
    echo "   --enable-lto"
    echo "   -j <numjobs>"
}


while [ $# -gt 0 ]; do
    case $1 in
        --build-dir)
            if [ -z "$2" ]; then
                usage "Error: Build dir has to be specified"
                exit 1
            fi
            builddir="$2"
            shift;
            ;;
        --install-prefix)
            if [ -z "$2" ]; then
                usage "Error: Install prefix has to be specified"
                exit 1
            fi
            install_prefix="$2"
            shift;
            ;;
        --barefoot-internal)
            barefoot_internal="-DENABLE_BAREFOOT_INTERNAL=ON"
            ;;
        --enable-cb)
            enable_cb="-DENABLE_CLOUDBREAK=ON"
            ;;
        --enable-fr)
            enable_fr="-DENABLE_FLATROCK=ON"
            ;;
        --enable-pgo)
            pgo=true
            ;;
        --enable-lto)
            lto=true
            ;;
        -j)
            parallel_make="$2"
            shift;
            ;;
        -h|--help)
            usage ""
            exit 0
            ;;
    esac
    shift
done

# This script is used by SDE CI, which is based on Ubuntu 16.04 and contains quite old version of cmake,
# unsupported by p4c frontend. Install CMake v3.16.3 (the version used in p4factory builds) to get newer cmake.
# TODO: Remove this workaround when SDE CI drops Ubuntu 16.04.
install_cmake
export CMAKE=/bfn/cmake/bin/cmake

# Add copyright headers to all BFN/Intel source files before building and
# packaging, so that we can easily avoid touching files from P4.org.
#
# Do this by adding headers to everything except p4c/ and p4-tests/.
"${topdir}/scripts/packaging/copyright-stamp" "${topdir}"

if $pgo ; then
    # Build an instrumented binary for PGO.
    build_p4c "-fprofile-generate"

    # Collect profiles by running a representative workload.
    ./scripts/internal/pgo_train.sh $builddir

    # Use the collected profiles to apply PGO.
    # TODO: With GCC 10 and newer, consider to also use -fprofile-partial-training
    build_p4c "-fprofile-use"
else
    build_p4c
fi

# Generate the final CPack archive.
(cd $builddir; make package)

echo "Success!"
exit 0
