# This file defines how a test should be written for a particular target. This is used by testutils

# Write the script to check eBPF Kernel STF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - p4test is the file that is to be tested.
macro(check_with_kernel testfile testfolder p4test runner_args)
  set(__p4cebpfpath "${P4C_BINARY_DIR}/p4c-ebpf")
  set(__ebpfrunner "${CMAKE_BINARY_DIR}/run-ebpf-test.py")
  # Find all the stf tests generated for this P4 file and test them with the eBPF kernel.
  file(APPEND ${testfile} "stffiles=($(find ${testfolder} -name \"*.stf\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${__testfile} "\tpython3 ${__ebpfrunner} ${runner_args} -t kernel -c ${__p4cebpfpath} -tf \${item} ${P4C_SOURCE_DIR} ${p4test}\n")
  file(APPEND ${testfile} "done\n")
endmacro(check_with_kernel)

# Add a single test to the testsuite.
# Arguments:
#   - TAG is a label for the set of test suite where this test belongs
#     (for example, p4ctest)
#   - DRIVER is the script that is used to run the test and compare the results
#   - ALIAS is a possibly different name for the test such that the
#     same p4 program can be used in different test configurations.
#     Must be unique across the test suite.
#   - P4TEST is the name of the p4 program to test (path relative to the p4c directory)
#   - TARGET is the target to test against
#   - ARCH is the p4 architecture
#   - ENABLE_RUNNER is the flag to  execute eBPF Kernel on the generated tests.
#   - TEST_ARGS is a list of arguments to pass to the test
#   - CMAKE_ARGS are additional arguments to pass to the test
#
# It generates a ${p4test}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 300s. For the slow CI machines.
macro(p4tools_add_test_with_args)
  # Parse arguments.
  set(options ENABLE_RUNNER)
  set(oneValueArgs TAG DRIVER ALIAS P4TEST TARGET ARCH)
  set(multiValueArgs TEST_ARGS CMAKE_ARGS RUNNER_ARGS)
  cmake_parse_arguments(
    TOOLS_EBPF_TESTS "${options}" "${oneValueArgs}"
    "${multiValueArgs}" ${ARGN}
  )
  # Set some lowercase variables for convenience.
  set(tag ${TOOLS_EBPF_TESTS_TAG})
  set(driver ${TOOLS_EBPF_TESTS_DRIVER})
  set(alias ${TOOLS_EBPF_TESTS_ALIAS})
  set(p4test ${TOOLS_EBPF_TESTS_P4TEST})
  set(target ${TOOLS_EBPF_TESTS_TARGET})
  set(arch ${TOOLS_EBPF_TESTS_ARCH})
  set(test_args ${TOOLS_EBPF_TESTS_TEST_ARGS})
  set(cmake_args ${TOOLS_EBPF_TESTS_CMAKE_ARGS})
  set(runner_args ${TOOLS_EBPF_TESTS_RUNNER_ARGS})
  # This is the actual test processing.
  p4c_test_set_name(__testname ${tag} ${alias})
  string(REGEX REPLACE ".p4" "" aliasname ${alias})
  set(__testfile "${P4TESTGEN_DIR}/${tag}/${alias}.test")
  set(__testfolder "${P4TESTGEN_DIR}/${tag}/${aliasname}.out")
  get_filename_component(__testdir ${p4test} DIRECTORY)
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")
  file(
    APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
    "${test_args} --out-dir ${__testfolder} \"$@\" ${p4test}\n"
  )
  # If ENABLE_RUNNER is active, run the generated tests on the eBPF kernek.
  if(${TOOLS_EBPF_TESTS_ENABLE_RUNNER})
    check_with_kernel(${__testfile} ${__testfolder} ${p4test} "${runner_args}")
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
  set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
endmacro(p4tools_add_test_with_args)
