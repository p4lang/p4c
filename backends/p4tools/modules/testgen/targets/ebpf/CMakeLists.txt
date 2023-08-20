if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding p4testgen ebpf test suite")
  include(test/P4Tests.cmake)
endif()

# Source files for p4testgen.
set(
  TESTGEN_SOURCES
  ${TESTGEN_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/backend/stf/stf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ebpf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmd_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/concolic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/expr_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/program_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/table_stepper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_backend.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_spec.cpp
  PARENT_SCOPE
)

# Link the run-ebpf-test binary
execute_process(COMMAND ln -sfn ${P4C_SOURCE_DIR}/backends/ebpf/run-ebpf-test.py ${CMAKE_BINARY_DIR}/run-ebpf-test.py)

set(
  TESTGEN_LIBS ${TESTGEN_LIBS}
  PARENT_SCOPE
)
