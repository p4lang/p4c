if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding p4testgen pna test suite")
  include(test/P4Tests.cmake)
endif()

# Source files for p4testgen.
set(
  TESTGEN_SOURCES
  ${TESTGEN_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/backend/metadata/metadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/backend/ptf/ptf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dpdk/cmd_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dpdk/expr_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dpdk/program_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dpdk/table_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pna.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/concolic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared_cmd_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared_expr_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared_program_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared_table_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_backend.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_spec.cpp
  PARENT_SCOPE
)

set(
  TESTGEN_LIBS ${TESTGEN_LIBS}
  PARENT_SCOPE
)
