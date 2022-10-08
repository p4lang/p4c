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

return_status=0

# Add local bin to the path in case clang-format is installed there.
export PATH=$PATH:${HOME}/.local/bin

find -L ${THIS_DIR}/../../common -iname '*.h' -o -iname '*.cpp' | xargs clang-format $write_args -i
return_status=$(($return_status || $?))
find -L ${THIS_DIR}/../../p4check -iname '*.h' -o -iname '*.cpp' | xargs clang-format $write_args -i
return_status=$(($return_status || $?))
find -L ${THIS_DIR}/../../testgen -iname '*.h' -o -iname '*.cpp' | xargs clang-format $write_args -i
return_status=$(($return_status || $?))


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
