#!/bin/sh
set -e
wget http://download.nanomsg.org/nanomsg-0.5-beta.tar.gz
tar -xzvf nanomsg-0.5-beta.tar.gz
cd nanomsg-0.5-beta
./configure
# ./configure --prefix=$HOME/nanomsg
make && sudo make install
cd ..
