# This file defines how a test should be written for a particular target. This is used by testutils

# add a single test to the testsuite
# Arguments:
#   - tag is a label for the set of test suite where this test belongs
#     (for example, p4ctest)
#   - driver is the script that is used to run the test and compare the results
#   - isXfail is boolean that specifies whether this test is expected to fail
#   - alias is a possibly different name for the test such that the
#     same p4 program can be used in different test configurations.
#     Must be unique across the test suite.
#   - p4test is the name of the p4 program to test (path relative to the p4c directory)
#   - target is the target to test against
#   - arch is the p4 architecture
#   - std is the p4 standard
#     the input packet
#   - enable_runner is the flag to compile the p4 and run the runner
#   - test_args is a list of arguments to pass to the test
#   - cmake_args are additional arguments to pass to the test
#
# It generates a ${p4test}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 300s (for the slow Travis machines)
#
macro(p4tools_add_test_with_args tag driver isXfail alias p4test target arch std enable_runner test_args cmake_args)
  p4c_test_set_name(__testname ${tag} ${alias})
  string(REGEX REPLACE ".p4" "" aliasname ${alias})
  set(__testfile "${P4TESTGEN_DIR}/${tag}/${alias}.test")
  set(__testfolder "${P4TESTGEN_DIR}/${tag}/${aliasname}.out")
  get_filename_component(__testdir ${P4C_SOURCE_DIR}/${p4test} DIRECTORY)
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4TOOLS_BINARY_DIR}\n")
  file(
    APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
    "--std ${std} ${test_args} --out-dir ${__testfolder} \"$@\" ${P4C_SOURCE_DIR}/${p4test}\n"
  )
  if(${enable_runner})
    set(__p4cbmv2path "${P4C_BINARY_DIR}")
    set(__bmv2runner "${CMAKE_BINARY_DIR}/run-bmv2-test.py")
    # Find all the stf tests generated for this p4 and copy them to the path where the p4 is, and test them with bmv2 model
    file(APPEND ${__testfile} "stffiles=($(find ${__testfolder} -name \"*.stf\"  | sort -n ))\n")
    file(APPEND ${__testfile} "for item in \${stffiles[@]}\n")
    file(APPEND ${__testfile} "do\n")
    file(APPEND ${__testfile} "\techo \"Found \${item}\"\n")
    file(APPEND ${__testfile} "\tpython3 ${__bmv2runner} . -v -b -tf \${item} -bd ${__p4cbmv2path} ${P4C_SOURCE_DIR}/${p4test}\n")
    file(APPEND ${__testfile} "done\n")
  endif()
  execute_process(COMMAND chmod +x ${__testfile})
  separate_arguments(__args UNIX_COMMAND ${cmake_args})
  add_test(
    NAME ${__testname}
    COMMAND ${tag}/${alias}.test ${__args}
    WORKING_DIRECTORY ${P4TESTGEN_DIR}
  )
  if(NOT DEFINED ${tag}_timeout)
    set(${tag}_timeout 300)
  endif()
  if(${isXfail})
    set_tests_properties(
      ${__testname} PROPERTIES LABELS "${tag};XFAIL"
      TIMEOUT ${${tag}_timeout} WILL_FAIL 1
    )
  else()
    set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
  endif()
endmacro(p4tools_add_test_with_args)
