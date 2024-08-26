# This file defines how to execute Smith on P4 programs.
# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)

set(SMITH_BMV2_CMD ${smith_SOURCE_DIR}/scripts/compilation-test.sh)

set(SMITH_BMV2_V1MODEL_ARGS 100 ${P4SMITH_DRIVER} ${CMAKE_BINARY_DIR}/p4c-bm2-ss ${CMAKE_BINARY_DIR} v1model bmv2)

add_test (NAME smith-compile-bmv2-v1model COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_V1MODEL_ARGS} WORKING_DIRECTORY ${P4C_BINARY_DIR})

set(SMITH_BMV2_PSA_ARGS 100 ${P4SMITH_DRIVER} ${CMAKE_BINARY_DIR}/p4c-bm2-psa ${CMAKE_BINARY_DIR} psa bmv2)

add_test (NAME smith-compile-bmv2-psa COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_PSA_ARGS} WORKING_DIRECTORY ${P4C_BINARY_DIR})
