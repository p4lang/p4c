if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding p4testgen tofino test suite")
  include(test/P4Tests.cmake)
endif()

# Source files for p4testgen.
set(
TESTGEN_SOURCES
${TESTGEN_SOURCES}

${CMAKE_CURRENT_SOURCE_DIR}/test_backend/ptf.cpp
${CMAKE_CURRENT_SOURCE_DIR}/test_backend/stf.cpp
${CMAKE_CURRENT_SOURCE_DIR}/compiler_result.cpp
${CMAKE_CURRENT_SOURCE_DIR}/concolic.cpp
${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hash_utils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/map_direct_externs.cpp
${CMAKE_CURRENT_SOURCE_DIR}/shared_expr_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/shared_program_info.cpp
${CMAKE_CURRENT_SOURCE_DIR}/shared_table_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
${CMAKE_CURRENT_SOURCE_DIR}/test_backend.cpp
${CMAKE_CURRENT_SOURCE_DIR}/test_spec.cpp

${CMAKE_CURRENT_SOURCE_DIR}/tofino/cmd_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tofino/expr_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tofino/program_info.cpp

${CMAKE_CURRENT_SOURCE_DIR}/tofino2/cmd_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tofino2/expr_stepper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tofino2/program_info.cpp

PARENT_SCOPE
)

# Override default settings in bf-p4c. We want those binaries to be built.
if (NOT TARGET p4cgraphs)
  set(ENABLE_P4C_GRAPHS ON CACHE BOOL "TRUE" FORCE)
  add_subdirectory(${P4C_SOURCE_DIR}/backends/graphs ${P4C_BINARY_DIR}/backends/graphs)
endif()

# Ideally, we'd link against just the Barefoot midend and a minimal set of
# transitive dependencies, but the compiler doesn't cleanly separate like
# that. Those transitive dependencies turn out to be most of the compiler!
set(
TESTGEN_LIBS
${TESTGEN_LIBS}
PARENT_SCOPE
)

# Custom IR constructs for P4Tools.
set(IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/testgen_tofino.def PARENT_SCOPE)
