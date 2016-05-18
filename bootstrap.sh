#!/bin/sh
# This command bootstraps the autoconf-based configuratino
# for the P4c compiler

set -e  # exit on error
./find-makefiles.sh # creates otherMakefiles.am, included in Makefile.am
mkdir -p extensions # place where additional back-ends are expected
echo "Running autoconf/configure tools"
autoreconf -i
mkdir -p build # recommended folder for build
sourcedir=`pwd`
cd build
# TODO: the "prefix" is needed for finding the p4include folder.
# It should be an absolute path.  This may need to change
# when we have a proper installation procedure.
../configure CXXFLAGS="-g -O1" --prefix=$sourcedir $*
echo "### Configured for building in 'build' folder"
