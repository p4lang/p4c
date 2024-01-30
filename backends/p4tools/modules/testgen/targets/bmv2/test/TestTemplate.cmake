# This file defines how a test should be written for a particular target. This is used by testutils

# Write the script to check BMv2 STF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - p4test is the file that is to be tested.
function(check_with_bmv2 testfile testfolder p4test)
  set(__p4cbmv2path "${P4C_BINARY_DIR}")
  set(__bmv2runner "${CMAKE_BINARY_DIR}/run-bmv2-test.py")
  # Find all the stf tests generated for this P4 file and test them with bmv2 model
  file(APPEND ${testfile} "stffiles=($(find ${testfolder} -name \"*.stf\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\tpython3 ${__bmv2runner} . -v -b -tf \${item} -bd ${__p4cbmv2path} ${p4test}\n")
  file(APPEND ${testfile} "done\n")
endfunction(check_with_bmv2)


# Write the script to check BMv2 PTF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - p4test is the file that is to be tested.
macro(check_bmv2_with_ptf testfile testfolder p4test)
  set(__p4cbmv2path "${P4C_BINARY_DIR}")
  set(__bmv2runner " ${P4C_SOURCE_DIR}/backends/bmv2/run-bmv2-ptf-test.py")
  # Find all the ptf tests generated for this P4 file and test them with bmv2 model
  file(APPEND ${testfile} "ptffiles=($(find ${testfolder} -name \"*.py\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${ptffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\t python3 ${__bmv2runner} --use-nanomsg -tf \${item} ${P4C_SOURCE_DIR}")
  file(APPEND ${testfile} " -pfn ${p4test} \n")
  file(APPEND ${testfile} "done\n")
endmacro(check_bmv2_with_ptf)

# Write the script to validate whether a given protobuf text format file has a valid format.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
function(validate_protobuf testfile testfolder)
  # Find all the proto tests generated for this P4 file and validate their correctness.
  file(APPEND ${testfile} "txtpbfiles=($(find ${testfolder} -name \"*.txtpb\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${txtpbfiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\t${PROTOC_BINARY} ${PROTOBUF_PROTOC_INCLUDES} -I${CMAKE_CURRENT_LIST_DIR}/../proto -I${P4RUNTIME_STD_DIR} -I${P4C_SOURCE_DIR}/control-plane --encode=p4testgen.TestCase p4testgen.proto < \${item} > /dev/null\n")
  file(APPEND ${testfile} "done\n")
endfunction(validate_protobuf)

# Write the script to validate whether a given protobuf IR text format file has a valid format.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
function(validate_protobuf_ir testfile testfolder)
  # Find all the proto tests generated for this P4 file and validate their correctness.
  file(APPEND ${testfile} "txtpbfiles=($(find ${testfolder} -name \"*.txtpb\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${txtpbfiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\t${PROTOC_BINARY} ${PROTOBUF_PROTOC_INCLUDES} -I${CMAKE_CURRENT_LIST_DIR}/../proto -I${P4RUNTIME_STD_DIR} -I${P4C_SOURCE_DIR} -I${P4C_SOURCE_DIR}/control-plane --encode=p4testgen_ir.TestCase p4testgen_ir.proto < \${item} > /dev/null\n")
  file(APPEND ${testfile} "done\n")
endfunction(validate_protobuf_ir)



# Write the script to validate whether a given protobuf file has a valid format.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
function(check_empty_folder testfile testfolder)
  file(APPEND ${testfile} "exit $(ls -A ${testfolder})\n")
endfunction(check_empty_folder)


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
#   - ENABLE_RUNNER is the flag to  execute BMv2 on the generated tests.
#   - VALIDATE_PROTOBUF is the flag to check whether the generated Protobuf tests are valid.
#   - VALIDATE_PROTOBUF is the flag to check whether the generated Protobuf IR tests are valid.
#   - TEST_ARGS is a list of arguments to pass to the test
#   - CMAKE_ARGS are additional arguments to pass to the test
#
# It generates a ${p4test}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 300s. For the slow CI machines.
function(p4tools_add_test_with_args)
  # Parse arguments.
  set(options ENABLE_RUNNER VALIDATE_PROTOBUF VALIDATE_PROTOBUF_IR P416_PTF USE_ASSERT_MODE DISABLE_ASSUME_MODE CHECK_EMPTY)
  set(oneValueArgs TAG DRIVER ALIAS P4TEST TARGET ARCH)
  set(multiValueArgs TEST_ARGS CMAKE_ARGS)
  cmake_parse_arguments(
    TOOLS_BMV2_TESTS "${options}" "${oneValueArgs}"
    "${multiValueArgs}" ${ARGN}
  )
  # Set some lowercase variables for convenience.
  set(tag ${TOOLS_BMV2_TESTS_TAG})
  set(driver ${TOOLS_BMV2_TESTS_DRIVER})
  set(alias ${TOOLS_BMV2_TESTS_ALIAS})
  set(p4test ${TOOLS_BMV2_TESTS_P4TEST})
  set(target ${TOOLS_BMV2_TESTS_TARGET})
  set(arch ${TOOLS_BMV2_TESTS_ARCH})
  set(test_args ${TOOLS_BMV2_TESTS_TEST_ARGS})
  set(cmake_args ${TOOLS_BMV2_TESTS_CMAKE_ARGS})

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

  if(${TOOLS_BMV2_TESTS_USE_ASSERT_MODE})
    set(test_args "${test_args} --assertion-mode")
  endif()
  if(${TOOLS_BMV2_TESTS_DISABLE_ASSUME_MODE})
    set(test_args "${test_args} --disable-assumption-mode")
  endif()

  file(
    APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
    "${test_args} --out-dir ${__testfolder} \"$@\" ${p4test}\n"
  )

  if(${TOOLS_BMV2_TESTS_USE_ASSERT_MODE} OR ${TOOLS_BMV2_TESTS_DISABLE_ASSUME_MODE})
    # Check whether the folder is empty.
    if(${TOOLS_BMV2_TESTS_CHECK_EMPTY})
      file(APPEND ${__testfile} "[ \"$(ls -A ${__testfolder})\" ] && exit 1 || exit 0")
    else()
      file(APPEND ${__testfile} "[ \"$(ls -A ${__testfolder})\" ] && exit 0 || exit 1")
    endif()
  else()
    # If ENABLE_RUNNER is active, run the BMv2 runner.
    if(${TOOLS_BMV2_TESTS_ENABLE_RUNNER})
      check_with_bmv2(${__testfile} ${__testfolder} ${p4test})
    endif()
    # If P416_PTF is active, run the PTF BMv2 runner.
    if(${TOOLS_BMV2_TESTS_P416_PTF})
      check_bmv2_with_ptf(${__testfile} ${__testfolder} ${p4test} ${__ptfRunerFolder})
    endif()
    # If VALIDATE_PROTOBUF is active, check whether the format of the generated tests is valid.
    if(${TOOLS_BMV2_TESTS_VALIDATE_PROTOBUF})
      validate_protobuf(${__testfile} ${__testfolder})
    endif()
    if(${TOOLS_BMV2_TESTS_VALIDATE_PROTOBUF_IR})
      validate_protobuf_ir(${__testfile} ${__testfolder})
    endif()
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
endfunction(p4tools_add_test_with_args)
