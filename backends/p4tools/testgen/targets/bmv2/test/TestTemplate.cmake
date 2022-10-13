# This file defines how a test should be written for a particular target. This is used by testutils


# Write the script to check BMv2 STF tests to the designated test file.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
#   - p4test is the file that is to be tested.
macro(check_with_bmv2 testfile testfolder p4test)
  set(__p4cbmv2path "${P4C_BINARY_DIR}")
  set(__bmv2runner "${CMAKE_BINARY_DIR}/run-bmv2-test.py")
  # Find all the stf tests generated for this P4 file and test them with bmv2 model
  file(APPEND ${testfile} "stffiles=($(find ${testfolder} -name \"*.stf\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\tpython3 ${__bmv2runner} . -v -b -tf \${item} -bd ${__p4cbmv2path} ${P4C_SOURCE_DIR}/${p4test}\n")
  file(APPEND ${testfile} "done\n")
endmacro(check_with_bmv2)

macro(check_with_bmv2_emi testfile testfolder p4test)
  set(__p4cbmv2path "${P4C_BINARY_DIR}")
  set(__bmv2runner "${CMAKE_BINARY_DIR}/run-bmv2-test.py")
  # Find all the stf tests generated for this p4 and copy them to the path where the p4 is, and test them with bmv2 model
  file(APPEND ${__testfile} "stffiles=($(find ${__testfolder} -name \"*.stf\"  | sort -n ))\n")
  file(APPEND ${__testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${__testfile} "do\n")
  file(APPEND ${__testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${__testfile} "\tpython3 ${__bmv2runner} . -v -b -tf \${item} -bd ${__p4cbmv2path} ${P4C_SOURCE_DIR}/${p4test} \n")
  file(APPEND ${__testfile} "\tfullpath=\"${p4test}\"\n")
  file(APPEND ${__testfile} "\tnamePart=($(echo \"\${item}\" | sed -r \"s/.+\\/(.+)\\..+/\\1/\"))\n")
  file(APPEND ${__testfile} "\tsubstring=\"${alias}\"\n")
  file(APPEND ${__testfile} "\tresultString=($(echo \${fullpath} | sed -e \"s/\${substring}$//\")) \n")
  file(APPEND ${__testfile} "\tsrcfiles=($(find ${P4C_SOURCE_DIR}/\${resultString} -name \"*\${namePart}_stf.p4\"  | sort -n ))\n")
  file(APPEND ${__testfile} "\tfor sourceTest in \${srcfiles[@]}\n")
  file(APPEND ${__testfile} "\tdo\n")
  file(APPEND ${__testfile} "\t\tif [[ \"\${sourceTest}\" != \"${P4C_SOURCE_DIR}/\${resultString}${alias}\" ]]\n")
  file(APPEND ${__testfile} "\t\tthen\n")
  file(APPEND ${__testfile} "\t\t\techo \"Found source test \${sourceTest}\"\n")
  file(APPEND ${__testfile} "\t\t\tpython3 ${__bmv2runner} . -v -b -tf \${item} -bd ${__p4cbmv2path} \${sourceTest} \n")
  file(APPEND ${__testfile} "\t\tfi\n")
  file(APPEND ${__testfile} "\tdone\n")
  file(APPEND ${__testfile} "done\n")
endmacro(check_with_bmv2_emi)

# Write the script to validate whether a given protobuf file has a valid format.
# Arguments:
#   - testfile is the testing script that this script is written to.
#   - testfolder is target folder of the test.
macro(validate_protobuf testfile testfolder)
  # Find all the proto tests generated for this P4 file and validate their correctness.
  file(APPEND ${testfile} "stffiles=($(find ${testfolder} -name \"*.proto\"  | sort -n ))\n")
  file(APPEND ${testfile} "for item in \${stffiles[@]}\n")
  file(APPEND ${testfile} "do\n")
  file(APPEND ${testfile} "\techo \"Found \${item}\"\n")
  file(APPEND ${testfile} "\tprotoc --proto_path=${CMAKE_CURRENT_LIST_DIR}/../proto --proto_path=${P4C_SOURCE_DIR}/control-plane/p4runtime/proto --proto_path=${P4C_SOURCE_DIR}/control-plane --encode=p4testgen.TestCase p4testgen.proto < \${item}\n")
  file(APPEND ${testfile} "done\n")
endmacro(validate_protobuf)


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
#   - TEST_ARGS is a list of arguments to pass to the test
#   - CMAKE_ARGS are additional arguments to pass to the test
#
# It generates a ${p4test}.test file invoking ${driver} on the p4
# program with command line arguments ${args}
# Sets the timeout on tests at 300s. For the slow CI machines.
macro(p4tools_add_test_with_args)
  # Parse arguments.
  set(options ENABLE_RUNNER VALIDATE_PROTOBUF EMI_TESTS)
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
  get_filename_component(__testdir ${P4C_SOURCE_DIR}/${p4test} DIRECTORY)
  file(WRITE ${__testfile} "#! /usr/bin/env bash\n")
  file(APPEND ${__testfile} "# Generated file, modify with care\n\n")
  file(APPEND ${__testfile} "set -e\n")
  file(APPEND ${__testfile} "cd ${P4TOOLS_BINARY_DIR}\n")
  # If EMI_TESTS is active, run the BMv2 runner.
  if(${TOOLS_BMV2_TESTS_EMI_TESTS})
    file(
      APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
      "--std p4-16 ${test_args} --generateSources 100 --out-dir ${__testfolder} \"$@\" ${P4C_SOURCE_DIR}/${p4test}\n"
    )
    set(${tag}_timeout 4000)
    check_with_bmv2_emi(${__testfile} ${__testfolder} ${p4test})
  else()
    file(
      APPEND ${__testfile} "${driver} --target ${target} --arch ${arch} "
      "--std p4-16 ${test_args} --out-dir ${__testfolder} \"$@\" ${P4C_SOURCE_DIR}/${p4test}\n"
    )
  endif()

  # If ENABLE_RUNNER is active, run the BMv2 runner.
  if(${TOOLS_BMV2_TESTS_ENABLE_RUNNER})
    check_with_bmv2(${__testfile} ${__testfolder} ${p4test})
  endif()
  # If VALIDATE_PROTOBUF is active, check whether the format of the generated tests is valid.
  if(${TOOLS_BMV2_TESTS_VALIDATE_PROTOBUF})
    validate_protobuf(${__testfile} ${__testfolder})
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
