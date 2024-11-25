# This file defines how a test should be written for a particular target. This is used by testutils

# Write the script to check Tofino STF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - testharness is binary to use for execution.
#   - aliasname is name of the test.
function(check_with_test_harness testfile testfolder testharness aliasname)
  if(NOT testharness)
    message(WARNING "STF tests need the test harness for ${target}. No entry was found.\n")
    return()
  endif()
  # Find all the stf tests generated for this p4 and run the test harness
  file(APPEND ${testfile} "stffiles=($(cd ${testfolder} && find * -name \"*.stf\"))\n")
  file(APPEND ${testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\t${testharness} -l ${testfolder}/${aliasname}.conf ${testfolder}/\${item}\n")
  file(APPEND ${testfile} "done\n")
endfunction(check_with_test_harness)

# Write the script to check Tofino STF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - p4test is the file that is to be tested.
function(check_with_model testfile testfolder target aliasname)
  set(__cmakecache "${CMAKE_BINARY_DIR}/CMakeCache.txt")
  set(__ptfrunner "${CMAKE_SOURCE_DIR}/p4-tests/ptf_runner.py")
  file(APPEND ${testfile} "if test -f \"${testfolder}/${aliasname}.py\"; then")
  file(APPEND ${testfile} "\techo \"Starting PTF: ${CMAKE_SOURCE_DIR}/p4-tests/run-isolated python3 ${__ptfrunner} --testdir ${testfolder} --name ${aliasname} --ptfdir ${testfolder} --top-builddir ${__cmakecache} --device ${target} --bfrt-test ${testfolder}/${aliasname}.conf\"\n")
  file(APPEND ${testfile} "\t${CMAKE_SOURCE_DIR}/p4-tests/run-isolated python3 ${__ptfrunner} --testdir ${testfolder} --name ${aliasname} --ptfdir ${testfolder} --top-builddir ${__cmakecache} --device ${target} --bfrt-test ${testfolder}/${aliasname}.conf\n")
  file(APPEND ${testfile} "fi")
endfunction(check_with_model)


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
#   - RUN_STF is the flag to  execute the Tofino test harness on the generated tests.
#   - RUN_PTF is the flag to  execute the Tofino model on the generated tests.
#   - TEST_ARGS is a list of arguments to pass to the test
#   - CMAKE_ARGS are additional arguments to pass to the test
#   - COMPILE_ARGS are additional compilation arguments to pass to p4c-barefoot
#
# It generates a ${p4test}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 1500. For the slow CI machines.
macro(p4tools_add_test_with_args)
  # Parse arguments.
  set(options RUN_STF RUN_PTF)
  set(oneValueArgs TAG DRIVER ALIAS P4TEST TARGET ARCH)
  set(multiValueArgs TEST_ARGS CMAKE_ARGS CTEST_P4C_ARGS)
  cmake_parse_arguments(
    TOOLS_TOFINO_TESTS "${options}" "${oneValueArgs}"
    "${multiValueArgs}" ${ARGN}
  )
  # Set some lowercase variables for convenience.
  set(tag ${TOOLS_TOFINO_TESTS_TAG})
  set(driver ${TOOLS_TOFINO_TESTS_DRIVER})
  set(alias ${TOOLS_TOFINO_TESTS_ALIAS})
  set(p4test ${TOOLS_TOFINO_TESTS_P4TEST})
  set(target ${TOOLS_TOFINO_TESTS_TARGET})
  set(arch ${TOOLS_TOFINO_TESTS_ARCH})
  set(test_args ${TOOLS_TOFINO_TESTS_TEST_ARGS})
  set(ctest_p4c_args ${TOOLS_TOFINO_TESTS_CTEST_P4C_ARGS})
  set(cmake_args ${TOOLS_TOFINO_TESTS_CMAKE_ARGS})

  # This is the actual test processing.
  p4c_test_set_name(__testname ${tag} ${alias})
  string(REPLACE ".p4" "" aliasname ${alias})
  set(__testfile "${P4TESTGEN_DIR}/${tag}/${alias}.test")
  set(__testfolder "${P4TESTGEN_DIR}/${tag}/${aliasname}.out")
  get_filename_component(__testdir ${p4test} DIRECTORY)
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4C_BINARY_DIR}\n")
  file(
    APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
    "--std p4-16 ${test_args} --out-dir ${__testfolder} \"$@\" ${p4test}\n"
  )
  # Compile the p4 file.
  file(APPEND ${__testfile} "\techo \"Compiling ${p4test}...\"\n")
  file(
    APPEND ${__testfile} "${CMAKE_BINARY_DIR}/p4c-barefoot --target ${target} --arch ${arch} "
    "--std p4-16 ${p4test} -I${CMAKE_SOURCE_DIR}/p4-tests/p4_16/includes "
    "-o ${__testfolder} ${ctest_p4c_args} || (echo Compiler failed && false)\n"
  )
  # FIXME: Reenable.
  # if(${TOOLS_TOFINO_TESTS_RUN_STF})
  #   # RUN_STF
  #   # Manually overwrite the tofino2 target as jbay, since tofino2 is not listed as variable.
  #   if(${target} STREQUAL "tofino2")
  #     get_property(testharness CACHE HARLYN_STF_jbay PROPERTY VALUE)
  #   elseif(${target} STREQUAL "tofino5")
  #     get_property(testharness CACHE HARLYN_STF_ftr PROPERTY VALUE)
  #   else()
  #     get_property(testharness CACHE HARLYN_STF_${target} PROPERTY VALUE)
  #   endif()
  #   if (testharness)
  #     check_with_test_harness(${__testfile} ${__testfolder} ${testharness} ${aliasname})
  #   else()
  #     message(WARNING "STF tests need the test harness for ${target}. No entry was found.\n")
  #   endif()
  # elseif(${TOOLS_TOFINO_TESTS_RUN_PTF})
  #   # Run PTF
  #   check_with_model(${__testfile} ${__testfolder} ${target} ${aliasname})
  # endif()

  execute_process(COMMAND chmod +x ${__testfile})
  separate_arguments(__args UNIX_COMMAND ${cmake_args})
  add_test(
    NAME ${__testname}
    COMMAND ${tag}/${alias}.test ${__args}
    WORKING_DIRECTORY ${P4TESTGEN_DIR}
  )
  if(NOT DEFINED ${tag}_timeout)
    set(${tag}_timeout 1500)
  endif()
  set_tests_properties(${__testname} PROPERTIES LABELS ${tag} TIMEOUT ${${tag}_timeout})
endmacro(p4tools_add_test_with_args)
