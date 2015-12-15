#!/bin/bash

THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

ROOT_DIR=$THIS_DIR/..

return_status=0

function run_cpplint() {
    # $1 is directory
    # $2 is root
    $THIS_DIR/cpplint.py --root=$2 $( find $1 -name \*.h -or -name \*.cpp )
    return_status=$(($return_status || $?))
}

run_cpplint $ROOT_DIR/modules/bm_sim/ modules
run_cpplint $ROOT_DIR/modules/bm_apps/ modules
run_cpplint $ROOT_DIR/targets/simple_switch targets
run_cpplint $ROOT_DIR/targets/l2_switch targets
run_cpplint $ROOT_DIR/targets/simple_router targets

exit $return_status
