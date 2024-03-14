#!/bin/bash
# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e  # exit on error

if type -p grealpath >/dev/null 2>/dev/null; then
    realpath=grealpath
else
    realpath=realpath
fi

mydir=`dirname $0`
mydir=`$realpath $mydir`
cd $mydir

BuildType=Debug
BuildDir=""
BuildGenerator="Unix Makefiles"
EXTRA=""
FORCE=false

while [ $# -gt 0 ]; do
    case "$1" in
    --build-dir)
        BuildDir="$2"
        shift
        ;;
    --build-type)
        BuildType="$2"
        shift
        ;;
    --build-generator)
        BuildGenerator="$2"
        shift
        ;;
    --distcc)
        export CC="distcc gcc"
        export CXX="distcc g++"
        ;;
    --disable-unified)
        EXTRA="$EXTRA -DENABLE_UNIFIED_COMPILATION=OFF"
        ;;
    --force)
        FORCE=true
        ;;
    debug|release|relwithdebinfo|Debug|Release|RelWithDebInfo|DEBUG|RELEASE|RELWITHDEBINFO)
        BuildType="$1"
        ;;
    build*)
        if [ -n "$BuildDir" ]; then
            echo >&2 "mulitple build directories not supported"
            exit 2
        elif [ -e "$1" ]; then
            if $FORCE; then
                rm -f "$1"/CMakeCache.txt
            else
                echo >&2 "$1 already exists, use --force to replace"
                exit 2
            fi
        fi
        BuildDir="$1"
        ;;
    -DCMAKE_BUILD_TYPE=*)
        BuildType="${1#-*=}"
        ;;
    -D*)
        EXTRA="$EXTRA $1"
        ;;
    *)
        echo >&2 "Invalid argument $1"
        exit 2
        ;;
    esac
    shift
done

if [ -z "$BuildDir" ]; then
    BuildDir=build    # default (recommended) folder for build
    if [ -e "$BuildDir" ]; then
        if $FORCE; then
            rm -f "$BuildDir"/CMakeCache.txt
        else
            echo >&2 "$BuildDir already exists, use --force to replace"
            exit 2
        fi
    fi
fi

mkdir -p p4c/extensions
mkdir -p "$BuildDir"
cd "$BuildDir"
cmake .. -DCMAKE_BUILD_TYPE=$BuildType -G "$BuildGenerator" $EXTRA
echo "### Configured for building with '$BuildGenerator' in '$BuildDir' folder"

make_relative_link ()
{
    local target="${1:?make_relative_link: Argument 1 should be the target.}"
    local link_name="${2:?make_relative_link: Argument 2 should be the link name.}"
    local link_name_dir="$(dirname "${link_name}")"
    mkdir -p "${link_name_dir}/"
    ln -sf "$($realpath --relative-to "${link_name_dir}/" "${target}")" "${link_name}"
}

make_relative_link "${mydir}/.gdbinit" "backends/bmv2/p4c-bm2-psa-gdb.gdb"
make_relative_link "${mydir}/.gdbinit" "backends/bmv2/p4c-bm2-ss-gdb.gdb"
make_relative_link "${mydir}/.gdbinit" "backends/dpdk/p4c-dpdk-gdb.gdb"
make_relative_link "${mydir}/.gdbinit" "backends/p4test/p4test-gdb.gdb"
