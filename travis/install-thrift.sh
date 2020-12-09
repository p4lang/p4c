#!/bin/sh

THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $THIS_DIR/common.sh

check_lib libthrift libthrift-0.11.0

set -e
# Make it possible to get thrift in China
# wget http://archive.apache.org/dist/thrift/0.11.0/thrift-0.11.0.tar.gz
# tar -xzvf thrift-0.11.0.tar.gz
git clone -b 0.11.0 https://github.com/apache/thrift.git thrift-0.11.0
cd thrift-0.11.0
./bootstrap.sh
./configure --with-cpp=yes --with-c_glib=no --with-java=no --with-ruby=no --with-erlang=no --with-go=no --with-nodejs=no
make -j2 && sudo make install
cd lib/py
sudo python3 setup.py install
cd ../../..
