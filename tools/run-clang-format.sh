#!/bin/bash


args=$(getopt -l "write" -o "wh" -- "$@")

eval set -- "$args"

while [ $# -ge 1 ]; do
        case "$1" in
                --)
                    # No more options left.
                    shift
                    break
                   ;;
                -w|--write)
                        write=1
                        shift
                        ;;
                -h)
                        echo "Options:"\
                        "-w/--write: Write the clang-format output instead of just displaying it."
                        exit 0
                        ;;
        esac

        shift
done

if [[ "${write}" -eq 1 ]] ; then
    write_args=""
else
    write_args="--Werror --dry-run"
fi

# This runs clang-format across the entire repository and the relevant source folders.
THIS_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ROOT_DIR=$THIS_DIR/..

return_status=0

# Add local bin to the path in case clang-format is installed there.
export PATH=$PATH:${HOME}/.local/bin

# For now, just run this on the tools back end.


EXCLUDE_DIRS="-path ${THIS_DIR}/../backends/p4tools/submodules\
             -o -path ${THIS_DIR}/../backends/ebpf/runtime\
             -o -path ${THIS_DIR}/../backends/ubpf/runtime\
             -o -path ${THIS_DIR}/../control-plane/p4runtime\
"

function run-clang-format() {
    # $1 is directory
    # $2 is root
return_status=$(($return_status || $?))
    lint_files=$(find $1 -type d \( ${EXCLUDE_DIRS} \) -prune -or -type f \( -iname \*.h -o -iname \*.cpp \) -print )
    echo $lint_files
    if [[ $lint_files ]]; then
        clang-format ${write_args} -i ${lint_files}
    fi
    return_status=$(($return_status || $?))
}

# TODO: Apply clang-format to all of these folders?

run-clang-format $ROOT_DIR/backends/p4tools
# run-clang-format $ROOT_DIR/backends
# run-clang-format $ROOT_DIR/control-plane
# run-clang-format $ROOT_DIR/frontends
# run-clang-format $ROOT_DIR/ir
# run-clang-format $ROOT_DIR/lib
# run-clang-format $ROOT_DIR/midend
# if [ -d "$DIRECTORY" ]; then
# run-clang-format $ROOT_DIR/extensions
# fi
# run-clang-format $ROOT_DIR/tools


echo "********************************"
if [[ "${write}" -eq 1 ]] ; then
    echo "CLANG-FORMAT APPLIED"
    exit $return_status
fi

if [ $return_status -eq 0 ]; then
    echo "CLANG-FORMAT CHECK SUCCESS"
else
    echo "CLANG-FORMAT CHECK FAILURE"
fi

exit $return_status
