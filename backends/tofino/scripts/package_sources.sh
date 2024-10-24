#! /bin/bash

set -x
set -e

topdir=$(readlink -f $(dirname $0)/..)
builddir=build.src_release
srcdir=bf-p4c_source

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
pgo=false
lto=false

cleanup_code() {
    cd $topdir
    ${topdir}/scripts/install_source_release_clean_prereqs.sh
    ${topdir}/scripts/source_release_clean.py
    # Do this by adding headers to everything except p4c/ and p4-tests/.
    "${topdir}/scripts/packaging/copyright-stamp" "${topdir}"
}

build_p4c() {
    ${topdir}/bootstrap_bfn_compilers.sh --build-dir $builddir \
                                --p4c-cpp-flags "$*" \
                                -DCMAKE_BUILD_TYPE=RELEASE \
                                -DCMAKE_INSTALL_PREFIX=$install_prefix \
                                -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_UBPF=OFF \
                                -DENABLE_P4TEST=ON -DENABLE_P4C_GRAPHS=OFF \
                                -DENABLE_DOXYGEN=OFF \
                                $enable_static \
                                $barefoot_internal \
                                -DENABLE_GTESTS=OFF \
                                -DENABLE_WERROR=OFF \
                                -DENABLE_ASSERTIONS=OFF \
                                -DENABLE_LTO=$lto

    (cd $builddir ; make -j $parallel_make)
}

run_test() {
    (cd $builddir/p4c ; cp bf-p4c p4c ; ctest -V -R "tofino/.*stf/.*decaf*|tofino32|tofino/.*ptf/hecksum*")
}

rm_extras() {
    echo $srcdir
    rm -rf $srcdir/ci
    rm -rf $srcdir/docker
    rm -rf $srcdir/jenkins
    rm -rf $srcdir/.dockerignore
    rm -rf $srcdir/ReleaseNotes.md
    rm -rf $srcdir/.travis.yml
    rm -rf $srcdir/Jenkinsfile
    rm -rf $srcdir/.git
    rm -rf $srcdir/.github
    rm -rf $srcdir/.gitignore
    rm -rf $srcdir/.gitmodules
    rm -rf $srcdir/install_os_deps.sh
    rm -rf $srcdir/scripts/_deps
    rm -rf $srcdir/scripts/internal
    rm -rf $srcdir/scripts/flatrock_utilities
    rm -rf $srcdir/scripts/gen_customer_reports
    rm -rf $srcdir/scripts/gen_reference_outputs
    rm -rf $srcdir/scripts/hooks
    rm -rf $srcdir/scripts/pkg-src
    rm -rf $srcdir/scripts/run_custom_tests
    rm -rf $srcdir/scripts/check-git-submodules
    rm -rf $srcdir/scripts/dot2pdf
    rm -rf $srcdir/scripts/jbay_rename.sh
    rm -rf $srcdir/scripts/p4c_build_for_arch.sh
    rm -rf $srcdir/scripts/travis_test
    rm -rf $srcdir/p4-tests/internal
    rm -rf $srcdir/p4-tests/p4_14/internal
    rm -rf $srcdir/p4-tests/p4_14/switch
    rm -rf $srcdir/p4-tests/p4_16/internal
    rm -rf $srcdir/p4-tests/p4_16/switch_16
    rm -rf $srcdir/p4-tests/p4testutils
    rm -rf $srcdir/bf-asm/test/internal
    rm -rf $srcdir/compiler_interfaces/.git
    rm -rf $srcdir/compiler_interfaces/.gitignore
    rm -rf $srcdir/compiler_interfaces/CODEOWNERS
    rm -rf $srcdir/bf-p4c_source
}

# Generate the final source archive
tar_sources() {
    mkdir -p $srcdir
    if ! which rsync > /dev/null ; then
        apt-get update && apt-get install -y rsync
    fi
    rsync -r --links --exclude=bf-p4c_source --exclude=build --exclude=.git . $srcdir
    rm_extras
    cd $topdir
    tar -czvf bf-p4c_source.tar.gz $srcdir
}

install_cmake() {
  wget https://cmake.org/files/v3.16/cmake-3.16.3-Linux-x86_64.tar.gz -q -O cmake.tar.gz
  mkdir -p /bfn/cmake
  tar zxf cmake.tar.gz --strip-components=1 -C /bfn/cmake
  rm cmake.tar.gz
}

usage() {
    echo $1
    echo "Usage: ./scripts/package_sources.sh <optional arguments>"
    echo "   --install-prefix <install_prefix>"
    echo "   -j <numjobs>"
}

while [ $# -gt 0 ]; do
    case $1 in
	-j)
            parallel_make="$2"
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

cleanup_code
build_p4c
run_test

rm -rf $builddir

tar_sources

echo "Success!"
exit 0
