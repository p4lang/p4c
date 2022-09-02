#!/bin/bash

### a simple test: ensure that a _single_ non-existant input pathname results in an error output that mentions that pathname.



source $(dirname "`realpath "${BASH_SOURCE[0]}"`")/driver_inputs_test___shared_code.bash

check_for_inadvisable_sourcing; returned=$?
if [ $returned -ne 0 ]; then return $returned; fi ### simulating exception handling for an exception that is not caught at this level



humanReadable_test_pathname="`resolve_symlink_only_of_basename "$0"`"



if ! P4C=`try_to_find_the_driver`; then
  echo "Unable to find the driver of the P4 compiler.  Aborting the test ''$humanReadable_test_pathname'' with a non-zero exit code.  This test failed." >& 2
  exit 255
fi
echo "In ''$humanReadable_test_pathname'', using ''$P4C'' as the path to the driver of the P4 compiler." >& 2



BAD_PATHNAME=/path/to/a/nonexistant/supposedly-P4/source/file

"$P4C" $BAD_PATHNAME 2>&1 | grep --ignore-case --quiet "error.*$BAD_PATHNAME"
exit_status_from_grep=$?

if [ $exit_status_from_grep -eq 0 ]; then
  echo "Test ''$humanReadable_test_pathname'' succeeded."
else
  echo "Test ''$humanReadable_test_pathname'' failed [grep returned $exit_status_from_grep]."
  exit 1
fi
