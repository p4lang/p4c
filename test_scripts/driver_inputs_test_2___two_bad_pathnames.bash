#!/bin/bash

### ensure that when _two_ non-existant input pathnames are given, each one results in an error output that mentions that pathname.



source $(dirname "`realpath "${BASH_SOURCE[0]}"`")/driver_inputs_test___shared_code.bash

check_for_inadvisable_sourcing; returned=$?
if [ $returned -ne 0 ]; then return $returned; fi ### simulating exception handling for an exception that is not caught at this level



BAD_PATHNAME_BASE=/path/to/a/nonexistant/supposedly-P4/source/file
### Technically, these don`t need to be _unique_ pathnames in order to trigger the bad/confusing behavior of the driver that Abe
### saw before he started working on it, but unique pathnames will more helpful for debugging, in case that will ever be needed.
BAD_PATHNAME_1=$BAD_PATHNAME_BASE/1
BAD_PATHNAME_2=$BAD_PATHNAME_BASE/2

### Using ASCII double quotes to guard against bugs due to ASCII spaces, even though this test-script file is free of such bugs as of this writing.
check_for_pathname_error "$BAD_PATHNAME_1" 1 2
result_1=$?
echo
check_for_pathname_error "$BAD_PATHNAME_2" 2 2
result_2=$?
echo



num_failures=0

if [ $result_1 -ne 0 ]; then num_failures=1; fi
if [ $result_2 -ne 0 ]; then num_failures=$(($num_failures+1)); fi

if [ $num_failures -ne 0 ]; then
  echo -n "$num_failures failure"
  if [ $num_failures -eq 1 ]; then
    echo -n ' was'
  else
    echo -n 's were'
  fi
  echo " detected in the test script ''$0''."
fi

### Clamping the exit code at a max. of 255, for future code that starts with a copy-paste from this file and may experience >255 errors
### [reminder: a positive multiple of 256 is a problem, since those all map to zero and zero means “everything was OK”].
if [ $num_failures -gt 255 ]; then num_failures=255; fi

exit $num_failures
