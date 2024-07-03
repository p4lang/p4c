# This file defines how to execute Smith on P4 programs.
# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)

set(SMITH_BMV2_CMD ${smith_SOURCE_DIR}/scripts/compilation-test.sh)

set(SMITH_BMV2_ARGS 100 ${P4SMITH_DRIVER} ${CMAKE_BINARY_DIR}/p4test ${CMAKE_BINARY_DIR} generic core)

set(P4_TEST_FILES
    smith_for_in_loop_test.cpp
    smith_for_loop_test.cpp
)

foreach(P4_TEST_FILE ${P4_TEST_FILES})
    string(REPLACE ".p4" "" TEST_NAME ${P4_TEST_FILE})
    add_test(NAME smith-compile-${TEST_NAME}
             COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/${P4_TEST_FILE}
             WORKING_DIRECTORY ${P4C_BINARY_DIR})
endforeach()

add_test(NAME smith-compile-core COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_ARGS} WORKING_DIRECTORY ${P4C_BINARY_DIR})


