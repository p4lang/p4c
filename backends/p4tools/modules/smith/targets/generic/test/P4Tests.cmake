# This file defines how to execute Smith on P4 programs.
# General test utilities.
include(${P4TOOLS_SOURCE_DIR}/cmake/TestUtils.cmake)

set(SMITH_BMV2_CMD ${smith_SOURCE_DIR}/scripts/compilation-test.sh)

set(SMITH_BMV2_ARGS 100 ${P4SMITH_DRIVER} ${CMAKE_BINARY_DIR}/p4test ${CMAKE_BINARY_DIR} core generic)

# GTest sources for p4smith (generic target).
set(P4SMITH_GENERIC_TARGET_GTEST_SOURCES
    ${P4C_SOURCE_DIR}/test/gtest/helpers.cpp
    ${P4C_SOURCE_DIR}/test/gtest/gtestp4c.cpp

    test/smith_for_in_loop_test.cpp
    test/smith_for_loop_test.cpp
)

if (ENABLE_GTESTS)
    add_executable(smith-compile-core ${P4SMITH_GENERIC_TARGET_GTEST_SOURCES})

    target_link_libraries(
        smith-compile-core
        PRIVATE smith
        PRIVATE gtest
        PRIVATE ${SMITH_LIBS}
        PRIVATE ${P4C_LIBRARIES}
        PRIVATE ${P4C_LIB_DEPS}
    )

    if (ENABLE_TESTING)
        add_test(NAME smith-compile-core COMMAND ${SMITH_BMV2_CMD} ${SMITH_BMV2_ARGS} WORKING_DIRECTORY ${P4C_BINARY_DIR})
        set_tests_properties(smith-compile-core PROPERTIES LABELS "gtest-smith-core")
    endif()

endif()

