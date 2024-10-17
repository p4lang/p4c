#! /bin/bash

# This script replicates the directory structure for p4c with
# extensions and invokes the p4lang/p4c/bootstrap to configure the
# build environment

umask 0022
set -e

mydir=`dirname $0`
mydir=`realpath $mydir`
pushd $mydir

show_help () {
    echo >&2 "bootstrap_bfn_compiler script options"
    echo >&2 "   --no-ptf               don't use/configure ptf"
    echo >&2 "   --build-dir <dir>      build in <dir> rather than ./build"
    echo >&2 "   --build-type <type>    DEBUG RELEASE or RelWithDebInfo"
    echo >&2 "   --p4c-cpp-flags <x>    add <x> to CPPFLAGS for p4c build"
    echo >&2 "   --enable-ubsan         build with undefined sanitizer"
    echo >&2 "   --enable-asan          build with address sanitizer"
    echo >&2 "   --small-config         only builds: (1) the compiler, (2) testing EXCEPT for the gtest-based tests"
    echo >&2 "   --minimal-config       disable most build targets other than the compiler"
    echo >&2 "   --disable-unity-build  disable unity build"
    echo >&2 "   --disable-werror       do not treat build warnings as errors"
    echo >&2 "   --cmake-gen <gen>      see 'cmake -h' for list of generators"
    echo >&2 "   --disable-preconfig    disable local pre-configured defaults even if they are present"
}


RUN_BOOTSTRAP_PTF=yes
builddir=${mydir}/build
buildtype=DEBUG
P4C_CPP_FLAGS=''
enableUBSan=false
enableASan=false
smallConfig=false
minimalConfig=false
disableUnityBuild=false
disableWerror=false
disablePreconfig=false
preconfigPath=/bfn/bf-p4c-preconfig.cmake
otherArgs=""
cmakeGen="Unix Makefiles"

while [ $# -gt 0 ]; do
    case $1 in
    --no-ptf)
        RUN_BOOTSTRAP_PTF=no
        ;;
    --build-dir)
        builddir="$2"
        shift
        ;;
    --build-type)
        buildtype="$2"
        shift
        ;;
    --p4c-cpp-flags)
        P4C_CPP_FLAGS="$2"
        shift
        ;;
    --enable-ubsan)
        enableUBSan=true
        ;;
    --enable-asan)
        enableASan=true
        ;;
    --small-config)
        smallConfig=true
        ;;
    --minimal-config)
        minimalConfig=true
        ;;
    --disable-unity-build)
        disableUnityBuild=true
        ;;
    --disable-werror)
        disableWerror=true
        ;;
    --cmake-gen)
        cmakeGen="$2"
        shift
        ;;
    --disable-preconfig)
        disablePreconfig=true
        shift
        ;;
    -D*)
        otherArgs+=" $1"
        ;;
    build-*)
        builddir="$1"
        ;;
    Debug|Release|RelWithDebInfo)
        buildtype="$1"
        ;;
    *)
        echo >&2 "Invalid argument supplied"
        show_help
        exit 0
        ;;
    esac
    shift
done

# Check for git version: 2.11.0 does not support virtual links
git_version=`git --version | head -1 | awk '{ print $3; }'`
if [[ $git_version == "2.11.0" ]]; then
    echo "git version $git_version is broken."
    echo "Please upgrade or downgrade your git version. Recommended versions: 1.8.x, 2.7.x, > 2.14.x"
    exit 1
fi

if [ ! -r p4c/CMakeLists.txt ]; then
    git submodule update --init --recursive
fi

# bootstrap the compiler
mkdir -p p4c/extensions

pushd p4c/extensions
if [ ! -e bf-p4c ]; then ln -sf ../../bf-p4c bf-p4c; fi
if [ ! -e p4_tests ]; then ln -sf ../../p4-tests p4_tests; fi
popd # p4c/extensions

# install a git hook if the user doesn't already have one defined
function install_hook() {
    local hook=$1
    if [[ -d .git && -e scripts/hooks/$hook && ! -e .git/hooks/$hook ]]; then
        pushd .git/hooks > /dev/null
        ln -sf ../../scripts/hooks/$hook $hook
        popd > /dev/null
    fi
}
install_hook pre-commit
install_hook commit-msg

ENABLED_COMPONENTS=""
if $smallConfig ; then
    ENABLED_COMPONENTS="$ENABLED_COMPONENTS -DENABLE_GTESTS=OFF"
fi
if $minimalConfig ; then
    ENABLED_COMPONENTS="$ENABLED_COMPONENTS -DENABLE_TESTING=OFF -DENABLE_GTESTS=OFF"
    ENABLED_COMPONENTS="$ENABLED_COMPONENTS -DENABLE_DOXYGEN=OFF"
fi
if $disableUnityBuild ; then
    ENABLED_COMPONENTS+=" -DCMAKE_UNITY_BUILD=OFF"
else
    ENABLED_COMPONENTS+=" -DCMAKE_UNITY_BUILD=ON"
fi

if $disableWerror ; then
    ENABLED_COMPONENTS+=" -DENABLE_WERROR=OFF"
fi

if [ "$disablePreconfig" = "false"  -a -r $preconfigPath ]; then
    otherArgs+=" -C$preconfigPath"
fi

if $enableUBSan ; then
    P4C_CPP_FLAGS+=" -fno-var-tracking-assignments -fsanitize=undefined"
fi

if $enableASan ; then
    P4C_CPP_FLAGS+=" -fno-var-tracking-assignments -fsanitize=address -fsanitize-recover=address"
    otherArgs+=" -DENABLE_GC=OFF"
fi

mkdir -p ${builddir}
pushd ${builddir}
( cat <<EOF
#/bin/bash
# CMake reconfiguration script. Run it in the build directory if content of
# CMakeLists has changed but there is no intention to change configuration.

${CMAKE:-cmake} ${mydir} -DCMAKE_BUILD_TYPE=${buildtype}\
      -G "${cmakeGen}" \
      ${ENABLED_COMPONENTS} \
      -DP4C_DRIVER_NAME='bf-p4c' \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1\
      -DP4C_CPP_FLAGS="$P4C_CPP_FLAGS" $otherArgs
if [[ \`uname -s\` == "Linux" ]]; then
    linux_distro=\$(cat /etc/os-release | grep -e "^NAME" | cut -f 2 -d '=' | tr -d "\"")
    linux_version=\$(cat /etc/os-release | grep -e "^VERSION_ID" | cut -f 2 -d '=' | tr -d "\"")
    if [[ \$linux_distro == "Fedora" && \$linux_version == "18" ]]; then
        # For some peculiar reason, Fedora 18 refuses to see the GC_print_stats symbol
        # even though cmake detects it. So we disabe it explicitly.
        sed -e 's/HAVE_GC_PRINT_STATS 1/HAVE_GC_PRINT_STATS 0/' -i"" p4c/config.h
    fi
fi
EOF
) > reconfigure
chmod +x reconfigure
./reconfigure
popd # builddir

if [ "$RUN_BOOTSTRAP_PTF" == "yes" ]; then
    ${mydir}/bootstrap_ptf.sh ${builddir}
fi

make_relative_link ()
{
    local target="${1:?make_relative_link: Argument 1 should be the target.}"
    local link_name="${2:?make_relative_link: Argument 2 should be the link name.}"
    local link_name_dir="$(dirname "${link_name}")"
    mkdir -p "${link_name_dir}/"
    ln -sf "$(realpath --relative-to "${link_name_dir}/" "${target}")" "${link_name}"
}

make_relative_link "${mydir}/bf-p4c/.gdbinit" "${builddir}/p4c/extensions/bf-p4c/p4c-barefoot-gdb.gdb"
make_relative_link "${mydir}/bf-p4c/.gdbinit" "${builddir}/p4c/backends/bmv2/p4c-bm2-ss-gdb.gdb"
make_relative_link "${mydir}/bf-p4c/.gdbinit" "${builddir}/p4c/backends/p4tests/p4test-gdb.gdb"
make_relative_link "${mydir}/bf-asm/.gdbinit" "${builddir}/bf-asm/bf-asm-gdb.gdb"

echo "Configured for build in ${builddir}"
popd > /dev/null # $mydir
