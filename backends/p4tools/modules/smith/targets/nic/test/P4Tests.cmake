# This file defines how to execute Smith on P4 programs.
# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)

set(SMITH_BMV2_CMD ${smith_SOURCE_DIR}/scripts/compilation-test.sh)

set(SMITH_BMV2_ARGS 100 ${P4SMITH_DRIVER} ${CMAKE_BINARY_DIR}/p4c-dpdk ${CMAKE_BINARY_DIR} dpdk pna)

add_test (NAME smith-compile-dpdk COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_ARGS} WORKING_DIRECTORY ${P4C_BINARY_DIR})

