# CMake file for P4Testgen.

include(common)

project(p4testgen)

# Declare common P4Testgen variables.
set(P4TESTGEN_DIR ${P4C_BINARY_DIR}/testgen)
set(P4TESTGEN_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4testgen")

# Source files for p4testgen.
set(
  TESTGEN_SOURCES
  options.cpp
  testgen.cpp

  core/externs.cpp
  core/program_info.cpp
  core/small_step/abstract_stepper.cpp
  core/small_step/cmd_stepper.cpp
  core/small_step/expr_stepper.cpp
  core/small_step/extern_stepper.cpp
  core/small_step/table_stepper.cpp
  core/small_step/small_step.cpp
  core/symbolic_executor/depth_first.cpp
  core/symbolic_executor/selected_branches.cpp
  core/symbolic_executor/random_backtrack.cpp
  core/symbolic_executor/greedy_stmt_cov.cpp
  core/symbolic_executor/max_stmt_cov.cpp
  core/symbolic_executor/symbolic_executor.cpp
  core/target.cpp

  lib/collect_coverable_nodes.cpp
  lib/concolic.cpp
  lib/continuation.cpp
  lib/execution_state.cpp
  lib/final_state.cpp
  lib/logging.cpp
  lib/packet_vars.cpp
  lib/test_backend.cpp
  lib/test_spec.cpp
  lib/tf.cpp
)

# GTest source files for p4testgen.
set(
  TESTGEN_GTEST_SOURCES
  #  # XXX These should be in a library.
  ${P4C_SOURCE_DIR}/test/gtest/helpers.cpp
  ${P4C_SOURCE_DIR}/test/gtest/gtestp4c.cpp

  test/gtest_utils.cpp
  test/lib/format_int.cpp
  test/lib/taint.cpp
  test/small-step/binary.cpp
  test/small-step/reachability.cpp
  test/small-step/unary.cpp
  test/small-step/util.cpp
  test/small-step/value.cpp
  test/small-step/p4_asserts_parser_test.cpp
  test/transformations/saturation_arithm.cpp
  test/z3-solver/asrt_model.cpp
  test/z3-solver/expressions.cpp
)

# Inja is needed to produce test templates.
set(INJA_BUILD_TESTS OFF CACHE BOOL "Build unit tests when BUILD_TESTING is enabled.")
set(BUILD_BENCHMARK OFF CACHE BOOL "Build benchmark.")
FetchContent_Declare(
  inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG        3741c73ba78babd2ed88f2acf2fcd6dafdb878e8
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(inja)

# Testgen libraries and includes. Defined here such that targets can extend them.
set(
  TESTGEN_LIBS
  PRIVATE p4tools-common
  PUBLIC inja
)
set(TESTGEN_INCLUDES)

file(GLOB testgen_targets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/targets ${CMAKE_CURRENT_SOURCE_DIR}/targets/*)
foreach(ext ${testgen_targets})
  set(testgen_targets_dir ${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext}/)
  if(EXISTS ${testgen_targets_dir}/CMakeLists.txt AND IS_DIRECTORY ${testgen_targets_dir})
    # Generate an option that makes it possible to disable this extension.
    string(MAKE_C_IDENTIFIER ${ext} EXT_AS_IDENTIFIER)
    string(TOUPPER ${EXT_AS_IDENTIFIER} EXT_AS_OPTION_NAME)
    string(CONCAT ENABLE_EXT_OPTION "ENABLE_TOOLS_TARGET_" ${EXT_AS_OPTION_NAME})
    string(CONCAT EXT_HELP_TEXT "Build the " ${ext} " target")
    option(${ENABLE_EXT_OPTION} ${EXT_HELP_TEXT} ON)
    if(${ENABLE_EXT_OPTION})
      message("-- Enabling target ${ext}")
      add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext})
      set(include_statements_var "${include_statements_var}#include \"backends/p4tools/modules/testgen/targets/${ext}/register.h\"\n")
      set(compiler_targets_var "${compiler_targets_var}    ${ext}RegisterCompilerTarget();\n")
      set(testgen_targets_var "${testgen_targets_var}    ${ext}RegisterTestgenTarget();\n")
    endif()
  endif()
endforeach(ext)

# Propagate def files set by target extensions upwards.
set(IR_DEF_FILES ${IR_DEF_FILES} PARENT_SCOPE)

# Fill the template
configure_file(register.h.in register.h)

# The library.
add_p4tools_library(testgen ${TESTGEN_SOURCES})
# Make sure the testgen library and anything that links to it see both the Z3 and Inja includes.
target_link_libraries(testgen PUBLIC p4tools-common inja)
target_include_directories(testgen ${TESTGEN_INCLUDES})

# The executable.
add_p4tools_executable(p4testgen main.cpp)
target_link_libraries(
  p4testgen
  PRIVATE testgen
  ${TESTGEN_LIBS}
  ${P4C_LIBRARIES}
  ${P4C_LIB_DEPS}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target(
  linkp4testgen
  # Add some convenience links for invoking p4testgen.
  # TODO: How to best handle includes?
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4testgen ${P4C_BINARY_DIR}/p4testgen
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
)

add_dependencies(p4testgen linkp4testgen)

if(ENABLE_GTESTS)
  add_executable(testgen-gtest ${TESTGEN_GTEST_SOURCES})
  target_include_directories(
    testgen-gtest
    PRIVATE ${P4C_SOURCE_DIR}/test/frameworks/gtest/googlemock/include
    PRIVATE ${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include
  )
  target_link_libraries(
    testgen-gtest
    PRIVATE testgen
    PRIVATE gtest
    ${TESTGEN_LIBS}
    ${P4C_LIBRARIES}
    ${P4C_LIB_DEPS}
    ${CMAKE_THREAD_LIBS_INIT}
  )

  if(ENABLE_TESTING)
    add_definitions("-DGTEST_HAS_PTHREAD=0")
    add_test(NAME testgen-gtest COMMAND testgen-gtest)
    set_tests_properties(testgen-gtest PROPERTIES LABELS "gtest-testgen")
  endif()

endif()
