#! /bin/bash

rootdir=$(realpath $(dirname $0)/..)
cd $rootdir

# # rename the published headers
git mv bf-p4c/arch/jna.cpp bf-p4c/arch/t2na.cpp
git mv bf-p4c/arch/jna.h bf-p4c/arch/t2na.h
git mv bf-p4c/p4include/jna.p4 bf-p4c/p4include/t2na.p4
git mv bf-p4c/p4include/jbay.p4 bf-p4c/p4include/tofino2.p4
git mv bf-p4c/driver/p4c.jbay.cfg bf-p4c/p4include/p4c.tofino2.cfg

echo "jna.p4 -> t2na.p4"
for f in `find $rootdir -name CMakeLists.txt -print -o -name "*.cpp" -print -o -name "*.p4" -print`; do
    sed -i "" -e "s/jna\.p4/t2na\.p4/" $f
done
echo "jbay.p4 -> tofino2.p4"
for f in `find $rootdir -name CMakeLists.txt -print -o -name "*.cpp" -print -o -name "*.p4" -print`; do
    sed -i "" -e "s/jbay\.p4/tofino2\.p4/" $f
done
echo "currentDevice() == \"Tofino\" -> currentDevice() == DEVICE::TOFINO"
for f in `find $rootdir/bf-p4c -name "*.cpp" -print -o -name "*.h" -print`; do
    sed -i "" -e "s/currentDevice\(\) == \"Tofino\"/currentDevice\(\) == Device::TOFINO/" $f
done

echo "currentDevice() == \"Jbay\" -> currentDevice() == DEVICE::JBAY"
for f in `find $rootdir/bf-p4c -name "*.cpp" -print -o -name "*.h" -print`; do
    sed -i "" -e 's/currentDevice\(\) == \"JBay\"/currentDevice\(\) == Device::JBAY/' $f
done

echo "options.target == \"tofino\" -> currentDevice() == DEVICE::TOFINO"
for f in `find $rootdir/bf-p4c -name "*.cpp" -print -o -name "*.h" -print`; do
    sed -i "" -e 's/options\.target == \"tofino\"/Device::currentDevice\(\) == Device::TOFINO/' $f
done

echo "options.target == \"jbay\" -> currentDevice() == DEVICE::JBAY"
for f in `find $rootdir/bf-p4c -name "*.cpp" -print -o -name "*.h" -print`; do
    sed -i "" -e 's/options\.target == \"jbay\"/Device\::currentDevice\(\) == Device::JBAY/' $f
done
