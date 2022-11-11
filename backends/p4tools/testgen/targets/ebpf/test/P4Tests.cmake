include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/TestUtils.cmake)

# Add eBPF tests from the p4c submodule
set(P4TOOLS_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(P4TESTGEN_DIR ${CMAKE_SOURCE_DIR}/build/testgen)
set(P4TESTGEN_DRIVER "${P4TOOLS_BINARY_DIR}/p4check testgen")

# This file defines how we write the tests we generate.
set(TEMPLATE_FILE ${CMAKE_CURRENT_LIST_DIR}/TestTemplate.cmake)

if(NOT P4TOOLS_EBPF_PATH)
  set(P4TOOLS_EBPF_PATH ${CMAKE_HOME_DIRECTORY}/build)
endif()

if(NOT NIGHTLY)
  set(EXTRA_OPTS "--strict --print-traces --seed 1000 --max-tests 10")
else()
  set(EXTRA_OPTS "--strict --print-traces --max-tests 10")
endif()

set(
  P4TESTGEN_EBPF_EXCLUDES
  "ebpf_checksum_extern\\.p4"
  "ebpf_conntrack_extern\\.p4"
)

set(EBPF_SEARCH_PATTERNS "include.*ebpf_model.p4")
set(P4TESTDATA ${P4C_SOURCE_DIR}/../p4c/testdata)
set(P4TESTS_FOR_EBPF "${P4TESTDATA}/p4_16_samples/*.p4")
p4c_find_tests("${P4TESTS_FOR_EBPF}" EBPF_TESTS INCLUDE "${EBPF_SEARCH_PATTERNS}" EXCLUDE "")
p4tools_find_tests("${EBPF_TESTS}" ebpftests EXCLUDE "${P4TESTGEN_EBPF_EXCLUDES}")


# Add ebpf tests from p4c
set(P4C_EBPF_TEST_SUITES_P416 ${ebpftests})

p4tools_add_tests(
  TESTSUITES "${P4C_EBPF_TEST_SUITES_P416}"
  TAG "testgen-p4c-ebpf" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "ebpf" ARCH "ebpf" ENABLE_RUNNER RUNNER_ARGS "" TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF ${EXTRA_OPTS} "
)

# These tests need special arguments.
p4tools_add_tests(
  TESTSUITES "${P4TESTDATA}/p4_16_samples/ebpf_checksum_extern.p4;"
  TAG "testgen-p4c-ebpf" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "ebpf" ARCH "ebpf" ENABLE_RUNNER RUNNER_ARGS "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-checksum-ebpf.c"
  TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF ${EXTRA_OPTS} "
)

p4tools_add_tests(
  TESTSUITES "${P4TESTDATA}/p4_16_samples/ebpf_conntrack_extern.p4;"
  TAG "testgen-p4c-ebpf" DRIVER ${P4TESTGEN_DRIVER} TEMPLATE_FILE ${TEMPLATE_FILE}
  TARGET "ebpf" ARCH "ebpf" ENABLE_RUNNER RUNNER_ARGS "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-conntrack-ebpf.c"
  TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF ${EXTRA_OPTS} "
)

#############################################################################
# TEST PROPERTIES
#############################################################################

include(${CMAKE_CURRENT_LIST_DIR}/EBPFXfail.cmake)
