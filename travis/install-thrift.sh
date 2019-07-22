#!/bin/sh

THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $THIS_DIR/common.sh

check_lib libthrift libthrift-0.9.2

set -e
# Make it possible to get thrift in China
# wget http://archive.apache.org/dist/thrift/0.9.2/thrift-0.9.2.tar.gz
# tar -xzvf thrift-0.9.2.tar.gz
git clone -b 0.9.2 https://github.com/apache/thrift.git thrift-0.9.2
cd thrift-0.9.2
./bootstrap.sh
./configure --with-cpp=yes --with-c_glib=no --with-java=no --with-ruby=no --with-erlang=no --with-go=no --with-nodejs=no
make -j2 && sudo make install
cd lib/py
sudo python setup.py install
cd ../../..
