# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

#############################################################################
# TEST PROGRAMS
#############################################################################
set(V1_SEARCH_PATTERNS "include.*v1model.p4" "main|common_v1_test")
# General BMv2 tests supplied by the compiler.
set(
  P4TESTS_FOR_BMV2 "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/dash/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/*.p4"
)
p4c_find_tests("${P4TESTS_FOR_BMV2}" P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")

# Custom BMv2 tests.
set(TESTGEN_BMV2_P416_TESTS "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4")
p4c_find_tests("${TESTGEN_BMV2_P416_TESTS}" BMV2_P4_16_V1_TESTS INCLUDE "${V1_SEARCH_PATTERNS}" EXCLUDE "")

# Add BMv2 tests from p4c and from testgen/test/p4-programs/bmv2
set(P4C_V1_TEST_SUITES_P416 ${P4_16_V1_TESTS} ${BMV2_P4_16_V1_TESTS})

#############################################################################
# TEST SUITES
#############################################################################
option(P4TOOLS_TESTGEN_BMV2_TEST_METADATA "Run tests on the Metadata test back end" ON)
option(P4TOOLS_TESTGEN_BMV2_TEST_PROTOBUF "Run tests on the Protobuf test back end" OFF)
option(P4TOOLS_TESTGEN_BMV2_TEST_PROTOBUF_IR "Run tests on the Protobuf test back end" ON)
option(P4TOOLS_TESTGEN_BMV2_TEST_PTF "Run tests on the PTF test back end" ON)
option(P4TOOLS_TESTGEN_BMV2_TEST_STF "Run tests on the STF test back end" ON)
# Test settings.
set(EXTRA_OPTS "--strict --print-traces --seed 1000 --max-tests 10 --track-coverage STATEMENTS --track-coverage TABLE_ENTRIES --track-coverage ACTIONS ")

# ASSERT_ASSUME TESTS
include(${CMAKE_CURRENT_LIST_DIR}/AssumeAssertTests.cmake)

# Protobuf
if(P4TOOLS_TESTGEN_BMV2_TEST_PROTOBUF)
  p4tools_add_tests(
    TESTS "${P4C_V1_TEST_SUITES_P416}"
    TAG "testgen-p4c-bmv2-protobuf" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "bmv2" ARCH "v1model" VALIDATE_PROTOBUF TEST_ARGS "--test-backend PROTOBUF ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/BMV2ProtobufXfail.cmake)
endif()

# Protobuf IR
if(P4TOOLS_TESTGEN_BMV2_TEST_PROTOBUF_IR)
  p4tools_add_tests(
    TESTS "${P4C_V1_TEST_SUITES_P416}"
    TAG "testgen-p4c-bmv2-protobuf-ir" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "bmv2" ARCH "v1model" VALIDATE_PROTOBUF_IR TEST_ARGS "--test-backend PROTOBUF_IR ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/BMV2ProtobufIrXfail.cmake)
endif()

# Metadata
if(P4TOOLS_TESTGEN_BMV2_TEST_METADATA)
  p4tools_add_tests(
    TESTS "${P4C_V1_TEST_SUITES_P416}"
    TAG "testgen-p4c-bmv2-metadata" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "bmv2" ARCH "v1model" TEST_ARGS "--test-backend METADATA ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/BMV2MetadataXfail.cmake)
endif()

# PTF
# TODO: The PTF test back end currently does not support packet sizes over 12000 bits, so we limit
# the range.
if(P4TOOLS_TESTGEN_BMV2_TEST_PTF)
  execute_process(COMMAND bash -c "printf \"import google.rpc\nimport google.protobuf\" | python3" RESULT_VARIABLE result)
  if(result AND NOT result EQUAL 0)
    message(
      WARNING
      "BMv2 PTF tests are enabled, but the Python3 module 'google.rpc' can not be found. BMv2 PTF tests will fail."
    )
  endif()
  # Filter some programs  because they have issues that are not captured with Xfails.
  set (P4C_V1_TEST_SUITES_P416_PTF ${P4C_V1_TEST_SUITES_P416})
  list(REMOVE_ITEM P4C_V1_TEST_SUITES_P416_PTF
       # A particular test (or packet?) combination leads to an infinite loop in the simple switch.
       "${P4C_SOURCE_DIR}/testdata/p4_16_samples/v1model-special-ops-bmv2.p4"
  )
  # Currently, the test back end only support ports 0-8.
  # TODO: Support the full range of ports.
  p4tools_add_tests(
    TESTS "${P4C_V1_TEST_SUITES_P416_PTF}"
    TAG "testgen-p4c-bmv2-ptf" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "bmv2" ARCH "v1model" P416_PTF TEST_ARGS "--test-backend PTF --packet-size-range 0:12000 --port-ranges 0:8 ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/BMV2PTFXfail.cmake)
endif()

# STF
if(P4TOOLS_TESTGEN_BMV2_TEST_STF)
  p4tools_add_tests(
    TESTS "${P4C_V1_TEST_SUITES_P416}"
    TAG "testgen-p4c-bmv2-stf" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "bmv2" ARCH "v1model" ENABLE_RUNNER TEST_ARGS "--test-backend STF ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/BMV2STFXfail.cmake)
endif()
