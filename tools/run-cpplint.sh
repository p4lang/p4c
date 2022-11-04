#!/bin/bash

# This runs cpplint across the entire repository and the relevant source folders.
THIS_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ROOT_DIR=$THIS_DIR/..

return_status=0

EXCLUDE_DIRS="-path ${THIS_DIR}/../backends/p4tools/submodules\
             -o -path ${THIS_DIR}/../backends/ebpf/runtime\
             -o -path ${THIS_DIR}/../backends/ubpf/runtime\
             -o -path ${THIS_DIR}/../control-plane/p4runtime\
"

function run_cpplint() {
    # $1 is directory
    # $2 is root
    lint_files=$(find $1 -type d \( ${EXCLUDE_DIRS} \) -prune -or -type f \( -iname \*.h -o -iname \*.cpp \))
    if [[ $lint_files ]]; then
        python3 $THIS_DIR/cpplint.py --root=$2 ${lint_files}
    fi
    return_status=$(($return_status || $?))
}

run_cpplint $ROOT_DIR/backends $ROOT_DIR
run_cpplint $ROOT_DIR/control-plane $ROOT_DIR
run_cpplint $ROOT_DIR/frontends $ROOT_DIR
run_cpplint $ROOT_DIR/ir $ROOT_DIR
run_cpplint $ROOT_DIR/lib $ROOT_DIR
run_cpplint $ROOT_DIR/midend $ROOT_DIR
if [ -d "$DIRECTORY" ]; then
run_cpplint $ROOT_DIR/extensions $ROOT_DIR
fi
run_cpplint $ROOT_DIR/tools $ROOT_DIR

echo "********************************"
if [ $return_status -eq 0 ]; then
    echo "CPPLINT CHECK SUCCESS"
else
    echo "CPPLINT CHECK FAILURE"
fi

exit $return_status
