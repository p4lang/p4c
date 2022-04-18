#!/bin/bash

### ensure that when _three_ non-existant input pathnames are given, each one results in an error output that mentions that pathname.



source $(dirname "`realpath "${BASH_SOURCE[0]}"`")/driver_inputs_test___shared_code.bash

check_for_inadvisable_sourcing; returned=$?
if [ $returned -ne 0 ]; then return $returned; fi ### simulating exception handling for an exception that is not caught at this level



humanReadable_test_pathname="`resolve_symlink_only_of_basename "$0"`"



if ! P4C=`try_to_find_the_driver`; then
  echo "Unable to find the driver of the P4 compiler.  Aborting the test ''$humanReadable_test_pathname'' with a non-zero exit code.  This test failed." >& 2
  exit 255
fi
echo "In ''$humanReadable_test_pathname'', using ''$P4C'' as the path to the driver of the P4 compiler." >& 2



BAD_PATHNAME_BASE=/path/to/a/nonexistant/supposedly-P4/source/file
### Technically, these don`t need to be _unique_ pathnames in order to trigger the bad/confusing behavior of the driver that Abe
### saw before he started working on it, but unique pathnames will more helpful for debugging, in case that will ever be needed.
BAD_PATHNAME_1=$BAD_PATHNAME_BASE/1
BAD_PATHNAME_2=$BAD_PATHNAME_BASE/2
BAD_PATHNAME_3=$BAD_PATHNAME_BASE/3

### Using ASCII double quotes to guard against bugs due to ASCII spaces, even though this test-script file is free of such bugs as of this writing.
check_for_pathname_error_in_P4_compiler_driver_output "$P4C" "$BAD_PATHNAME_1" 1 3
result_1=$?
echo
check_for_pathname_error_in_P4_compiler_driver_output "$P4C" "$BAD_PATHNAME_2" 2 3
result_2=$?
echo
check_for_pathname_error_in_P4_compiler_driver_output "$P4C" "$BAD_PATHNAME_3" 3 3
result_3=$?
echo



num_failures=0
if [ $result_1 -ne 0 ]; then num_failures=1                   ; fi
if [ $result_2 -ne 0 ]; then num_failures=$(($num_failures+1)); fi
if [ $result_3 -ne 0 ]; then num_failures=$(($num_failures+1)); fi

report___num_failures___and_clamp_it_to_an_inclusive_maximum_of_255

exit $num_failures
