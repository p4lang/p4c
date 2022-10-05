#!/bin/bash

# This runs cpplint across the entire repository and the relevant source folders.
THIS_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ROOT_DIR=$THIS_DIR/../..

return_status=0

function run_cpplint() {
    # $1 is directory
    # $2 is root
    python3 $THIS_DIR/cpplint.py --root=$2 $( find $1 -name \*.h -or -name \*.cpp )
    return_status=$(($return_status || $?))
}

run_cpplint $ROOT_DIR/common $ROOT_DIR
run_cpplint $ROOT_DIR/p4check $ROOT_DIR
run_cpplint $ROOT_DIR/testgen $ROOT_DIR

echo "********************************"
if [ $return_status -eq 0 ]; then
    echo "CPPLINT CHECK SUCCESS"
else
    echo "CPPLINT CHECK FAILURE"
fi

exit $return_status
