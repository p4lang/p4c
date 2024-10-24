#!/bin/bash

### a simple test: ensure that a _single_ non-existant input pathname results in an error output that mentions that pathname.

SCRIPT_NAME=$(basename "$0")
USAGE="Usage: $SCRIPT_NAME <path-to-driver-binary>"

show_usage() {
    echo "$USAGE"
}

if [ $# -eq 0 ]; then
    echo "Error: No path to driver binary provided."
    show_usage
    exit 1
fi

source $(dirname "`realpath "${BASH_SOURCE[0]}"`")/driver_inputs_test___shared_code.bash
check_for_inadvisable_sourcing; returned=$?
if [ $returned -ne 0 ]; then return $returned; fi ### simulating exception handling for an exception that is not caught at this level

driver_path="$1"
validate_driver_binary "$driver_path"

humanReadable_test_pathname="`resolve_symlink_only_of_basename "$0"`"


BAD_PATHNAME=/path/to/a/nonexistant/supposedly-P4/source/file

"$driver_path" $BAD_PATHNAME 2>&1 | grep --ignore-case --quiet "error.*$BAD_PATHNAME"
exit_status_from_grep=$?

if [ $exit_status_from_grep -eq 0 ]; then
  echo "Test ''$humanReadable_test_pathname'' succeeded."
else
  echo "Test ''$humanReadable_test_pathname'' failed [grep returned $exit_status_from_grep]."
  exit 1
fi
