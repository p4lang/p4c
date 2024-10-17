#! /bin/bash

set -e

if [ $# -ne 1 ]; then
    echo "This script expects exactly one argument, the path to the build dir"
    exit 1
fi

CC=cc
mydir=`dirname $0`
cd $mydir
ptf_runner_wrapper_SRC=$1/p4c/ptf_runner_wrapper.c
ptf_runner_wrapper=$1/p4c/extensions/p4_tests/ptf_runner_wrapper
$CC $ptf_runner_wrapper_SRC -o $ptf_runner_wrapper
sudo chown root $ptf_runner_wrapper
sudo chmod u+s $ptf_runner_wrapper
