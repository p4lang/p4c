#!/bin/sh
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

# This command bootstraps the autoconf-based configuration
# for the P4c compiler

set -e  # exit on error
./find-makefiles.sh # creates otherMakefiles.am, included in Makefile.am

# Generates unified compilation rules; needs to be done before automake runs.
./tools/autounify --max-chunk-size 10

mkdir -p extensions # place where additional back-ends are expected
echo "Running autoconf/configure tools"
rm -f aclocal.m4  # Needed to ensure we see updates to extension addconfig.ac files.
case "$(uname)" in
    Darwin) #MAC OS
        glibtoolize
        ;;
    *)
        libtoolize
        ;;
esac
autoreconf -i
mkdir -p build # recommended folder for build
sourcedir=`pwd`
cd build
../configure CXXFLAGS="-g -O0" --disable-doxygen-doc $*
echo "### Configured for building in 'build' folder"
