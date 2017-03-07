#!/bin/bash

THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

ROOT_DIR=$THIS_DIR/..

return_status=0

function run_cpplint() {
    # $1 is directory
    # $2 is root
    python2 $THIS_DIR/cpplint.py --root=$2 $( find $1 -name \*.h -or -name \*.cpp )
    return_status=$(($return_status || $?))
}

run_cpplint $ROOT_DIR/src/bm_sim src
run_cpplint $ROOT_DIR/src/bm_apps src
run_cpplint $ROOT_DIR/include/bm/bm_sim include
run_cpplint $ROOT_DIR/include/bm/bm_apps include
run_cpplint $ROOT_DIR/targets/simple_switch targets
run_cpplint $ROOT_DIR/targets/l2_switch targets
run_cpplint $ROOT_DIR/targets/simple_router targets

run_cpplint $ROOT_DIR/tests .

run_cpplint $ROOT_DIR/pdfixed/src src
run_cpplint $ROOT_DIR/pdfixed/include pdfixed/include

run_cpplint $ROOT_DIR/PI PI

exit $return_status
