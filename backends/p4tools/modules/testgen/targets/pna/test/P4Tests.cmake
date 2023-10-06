# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

#############################################################################
# TEST PROGRAMS
#############################################################################
set(PNA_SEARCH_PATTERNS "include.*pna.p4" "main")
# General PNA tests supplied by the compiler.
set(
  P4TESTS_FOR_PNA "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
)
p4c_find_tests("${P4TESTS_FOR_PNA}" PNA_TESTS INCLUDE "${PNA_SEARCH_PATTERNS}" EXCLUDE "")

# Custom PNA tests.
set(TESTGEN_PNA_P416_TESTS "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4")
p4c_find_tests("${TESTGEN_PNA_P416_TESTS}" CUSTOM_PNA_TESTS INCLUDE "${PNA_SEARCH_PATTERNS}" EXCLUDE "")

# Add PNA tests from P4C and from testgen/test/p4-programs/pna
set(PNA_TEST_SUITE ${PNA_TESTS} ${CUSTOM_PNA_TESTS})


#############################################################################
# TEST SUITES
#############################################################################
option(P4TOOLS_TESTGEN_PNA_TEST_METADATA "Run tests on the Metadata test back end" ON)
# Test settings.
set(EXTRA_OPTS "--strict --print-traces --seed 1000 --max-tests 10 ")

# Metadata
if(P4TOOLS_TESTGEN_PNA_TEST_METADATA)
  p4tools_add_tests(
    TESTS "${PNA_TEST_SUITE}"
    TAG "testgen-p4c-pna-metadata" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "dpdk" ARCH "pna" TEST_ARGS "--test-backend METADATA ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/PNAMetadataXfail.cmake)
endif()

