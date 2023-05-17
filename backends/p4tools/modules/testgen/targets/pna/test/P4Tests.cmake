# General test utilities.
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/TestUtils.cmake)
# This file defines how we write the tests we generate.
include(${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

#############################################################################
# TEST PROGRAMS
#############################################################################
set(PNA_SEARCH_PATTERNS "include.*pna.p4" "main")
# General PNa tests supplied by the compiler.
set(
  P4TESTS_FOR_PNA "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
)
p4c_find_tests("${P4TESTS_FOR_PNA}" PNA_TESTS INCLUDE "${PNA_SEARCH_PATTERNS}" EXCLUDE "")
p4tools_find_tests("${PNA_TESTS}" v1tests EXCLUDE "")

# Custom PNa tests.
set(TESTGEN_PNA_P416_TESTS "${CMAKE_CURRENT_LIST_DIR}/p4-programs/*.p4")
p4c_find_tests("${TESTGEN_PNA_P416_TESTS}" PNA_P4_16_PNA_TESTS INCLUDE "${PNA_SEARCH_PATTERNS}" EXCLUDE "")
p4tools_find_tests("${PNA_P4_16_PNA_TESTS}" pna_tests EXCLUDE "")

# Add PNa tests from p4c and from testgen/test/p4-programs/pna
set(P4C_V1_TEST_SUITES_P416 ${v1tests} ${pna_tests})


#############################################################################
# TEST SUITES
#############################################################################
option(P4TOOLS_TESTGEN_PNA_TEST_METADATA "Run tests on the Metadata test back end" ON)
# Test settings.
set(EXTRA_OPTS "--strict --print-traces --seed 1000 --max-tests 10 ")
set(P4TESTGEN_DRIVER "${P4TOOLS_BINARY_DIR}/p4testgen")

# Metadata
if(P4TOOLS_TESTGEN_PNA_TEST_METADATA)
  p4tools_add_tests(
    TESTSUITES "${P4C_V1_TEST_SUITES_P416}"
    TAG "testgen-p4c-pna-metadata" DRIVER ${P4TESTGEN_DRIVER}
    TARGET "dpdk" ARCH "pna" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend METADATA ${EXTRA_OPTS} "
  )
  include(${CMAKE_CURRENT_LIST_DIR}/PNAMetadataXfail.cmake)
endif()

