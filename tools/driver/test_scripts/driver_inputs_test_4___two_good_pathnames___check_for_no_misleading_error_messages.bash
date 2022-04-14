#!/bin/bash

### Ensure that when two _valid_ pathnames are given to the compiler driver,
###   it does not emit misleading error messages just because it can`t handle >1 [non-JSON] input file at a time.

### IMPORTANT NOTE: this test will probably need to be altered/removed if/when the compiler driver will ever support:
###    >1 P4   input at a time
### OR
###    >1 JSON input at a time
### OR
###     a P4 input and a JSON input at the same time withOUT the JSON input parameter simply overwriting the P4 input pathname in the parameter list.
###
### ... or if “p4include/core.p4” and/or “p4include/pna.p4” will ever _not_ be present at the top level of a valid build directory [of a built build].



source $(dirname "`realpath "${BASH_SOURCE[0]}"`")/driver_inputs_test___shared_code.bash

check_for_inadvisable_sourcing; returned=$?
if [ $returned -ne 0 ]; then return $returned; fi ### simulating exception handling for an exception that is not caught at this level



output_dir=`mktemp -d /tmp/P4C_driver_testing___XXXXXXXXXX`



echo '===== vvv ===== test debugging ===== vvv ====='
echo "''pwd'' result: ''`pwd`''"
echo
ls -dl p4c
echo
ls -dl p4c*
echo
ls -l 
echo
echo === env ===
env
echo
echo === set ===
set
echo
echo '===== ^^^ ===== test debugging ===== ^^^ ====='



humanReadable_test_pathname="`resolve_symlink_only_of_basename "$0"`"

if ! P4C=`try_to_find_the_driver`; then
  echo "Unable to find the driver of the P4 compiler.  Aborting the test ''$humanReadable_test_pathname'' with a non-zero exit code.  This test failed." >& 2
  exit 255
fi
echo "In ''$humanReadable_test_pathname'', using ''$P4C'' as the path to the driver of the P4 compiler." >& 2



if [ ! -x "$P4C" ] ; then
  echo "Test ''$humanReadable_test_pathname'' failed due to not being able to execute ''$P4C''." >& 2
  exit 1
elif "$P4C" p4include/core.p4 p4include/pna.p4 -o "$output_dir" 2>&1 | grep --ignore-case --quiet 'unrecognized.*arguments'; then
  echo "Test ''$humanReadable_test_pathname'' failed due to a required-to-not-be-present-in-the-output regex having been found/matched by ''grep''." >& 2
  exit 2
else
  echo "Test ''$humanReadable_test_pathname'' succeeded." >& 2
fi
