#!/bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

### Checks that the driver passes source, include, and output pathnames containing spaces through intact.

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
if [ $returned -ne 0 ]; then return $returned; fi

driver_path="$1"
check_driver_binary_exists "$driver_path"
check_driver_is_executable "$driver_path"

humanReadable_test_pathname="`resolve_symlink_only_of_basename "$0"`"

work_dir="`mktemp -d '/tmp/p4c driver quoting.XXXXXXXX'`"
trap 'rm -rf "$work_dir"' EXIT

src_dir="$work_dir/source dir"
inc_dir="$work_dir/include dir"
out_dir="$work_dir/output dir"
mkdir -p "$src_dir" "$inc_dir" "$out_dir"

printf '#define QUOTED_HEADER_MARKER quoted_header_was_found\n' > "$inc_dir/quoted definitions.p4"
printf '#include "quoted definitions.p4"\nQUOTED_HEADER_MARKER\n' > "$src_dir/prog.p4"

"$driver_path" --target bmv2 --arch v1model -E -I "$inc_dir" -o "$out_dir" "$src_dir/prog.p4"
driver_status=$?

if [ $driver_status -ne 0 ]; then
    echo "Test ''$humanReadable_test_pathname'' failed: the driver exited with status $driver_status." >& 2
    exit 1
fi

p4i_file="$out_dir/prog.p4i"
if [ ! -f "$p4i_file" ]; then
    echo "Test ''$humanReadable_test_pathname'' failed: the preprocessed output ''$p4i_file'' is missing." >& 2
    exit 1
fi

if ! grep -q 'quoted_header_was_found' "$p4i_file"; then
    echo "Test ''$humanReadable_test_pathname'' failed: the include directory with the space was not used." >& 2
    exit 1
fi

echo "Test ''$humanReadable_test_pathname'' succeeded."
