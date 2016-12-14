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

# This command bootstraps the autoconf-based configuratino
# for the P4c compiler

set -e  # exit on error
./find-makefiles.sh # creates otherMakefiles.am, included in Makefile.am

# Generate the unified compilation makefile, which is included in Makefile.am.
# This needs to be done before automake runs. The argument indicates the maximum
# number of files per unified compilation chunk. See the source code of the tool
# for more discussion.
echo "Generating unified-compilation.am"
tools/gen-unified-makefile.py 10 > unified-compilation.am

mkdir -p extensions # place where additional back-ends are expected
echo "Running autoconf/configure tools"
rm -f aclocal.m4  # Needed to ensure we see updates to extension addconfig.ac files.
autoreconf -i
mkdir -p build # recommended folder for build
sourcedir=`pwd`
cd build
# TODO: the "prefix" is needed for finding the p4include folder.
# It should be an absolute path.  This may need to change
# when we have a proper installation procedure.
../configure CXXFLAGS="-g -O0" --prefix=$sourcedir $*
echo "### Configured for building in 'build' folder"
