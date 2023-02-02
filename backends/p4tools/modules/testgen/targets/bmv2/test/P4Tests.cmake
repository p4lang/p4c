include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/TestUtils.cmake)

# Add v1model tests from the p4c submodule
set(P4TOOLS_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(P4TESTGEN_DIR ${CMAKE_SOURCE_DIR}/build/testgen)
set(P4TESTGEN_DRIVER "${P4TOOLS_BINARY_DIR}/p4testgen")

# This file defines how we write the tests we generate.
set(TEMPLATE_FILE ${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)


if(NOT P4TOOLS_BMV2_PATH)
  set(P4TOOLS_BMV2_PATH ${CMAKE_HOME_DIRECTORY}/build)
endif()

if(NOT NIGHTLY)
  set(EXTRA_OPTS "--strict --print-traces --seed 1000 --max-tests 10")
else()
  set(EXTRA_OPTS "--strict --print-traces --max-tests 10")
endif()

set(V1_SEARCH_PATTERNS "include.*v1model.p4" "main|common_v1_test")
set(P4TESTDATA ${P4C_SOURCE_DIR}/testdata)
set(P4TESTS_FOR_BMV2 "${P4TESTDATA}/p4_16_samples/*.p4")
p4c_find_tests("${P4TESTS_FOR_BMV2}" P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")
p4tools_find_tests("${P4_16_V1_TESTS}" v1tests EXCLUDE "")

set(
  TESTGEN_BMV2_P416_TESTS
  "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
)

p4c_find_tests("${TESTGEN_BMV2_P416_TESTS}" BMV2_P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")
p4tools_find_tests("${BMV2_P4_16_V1_TESTS}" bmv2v1tests EXCLUDE "")

# Add bmv2 tests from p4c and from testgen/test/p4-programs/bmv2
set(P4C_V1_TEST_SUITES_P416 ${v1tests} ${bmv2v1tests})

p4tools_add_tests(
  TESTSUITES "${P4C_V1_TEST_SUITES_P416}"
  TAG "testgen-p4c-bmv2" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "bmv2" ARCH "v1model" ENABLE_RUNNER TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF ${EXTRA_OPTS} "
)

#############################################################################
# TEST PROPERTIES
#############################################################################

# Add bmv2 PTF tests from p4c and from testgen/test/p4-programs/bmv2
set(P4C_V1_TEST_SUITES_P416_PTF ${bmv2v1tests})

execute_process(
  COMMAND bash -c "printf \"import google.rpc\nimport google.protobuf\" | python3" RESULT_VARIABLE result
)
if(result AND NOT result EQUAL 0)
  message(
    WARNING
      "BMv2 PTF tests are enabled, but the Python3 module 'google.rpc' can not be found. BMv2 PTF tests will fail."
  )
endif()

# TODO: The PTF test back end currently does not support packet sizes over 12000 bits, so we limit
# the range.
p4tools_add_tests(
  TESTSUITES "${P4C_V1_TEST_SUITES_P416}"
  TAG "testgen-p4c-bmv2-ptf" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "bmv2" ARCH "v1model" P416_PTF TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend PTF --packet-size-range 0:12000 ${EXTRA_OPTS} "
)

p4tools_add_tests(
  TESTSUITES "${P4C_V1_TEST_SUITES_P416}"
  TAG "testgen-p4c-bmv2-protobuf" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "bmv2" ARCH "v1model" VALIDATE_PROTOBUF TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend PROTOBUF ${EXTRA_OPTS} "
)

 include(${CMAKE_CURRENT_LIST_DIR}/BMV2Xfail.cmake)
 include(${CMAKE_CURRENT_LIST_DIR}/BMV2PTFXfail.cmake)
 include(${CMAKE_CURRENT_LIST_DIR}/BMV2ProtobufXfail.cmake)
