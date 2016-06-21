#!/bin/sh
set -e
git clone https://github.com/nanomsg/nnpy.git
# top of tree won't install
cd nnpy
git checkout c7e718a5173447c85182dc45f99e2abcf9cd4065
sudo pip install cffi
sudo pip install .
cd ..
