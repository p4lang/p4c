#!/bin/bash

THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

TEST_DIR=$THIS_DIR/../tests

cd $TEST_DIR

libtool --mode=execute valgrind --leak-check=full --show-reachable=yes ./test_all --valgrind
return_status=$?

cd -

exit $return_status
