# CMake file for Barefoot p4testgen.
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(p4testgen)

# Source files for p4testgen.
set(
  TESTGEN_SOURCES
  options.cpp
  testgen.cpp

  core/arch_spec.cpp
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
  core/symbolic_executor/linear_enumeration.cpp
  core/symbolic_executor/greedy_stmt_cov.cpp
  core/symbolic_executor/max_stmt_cov.cpp
  core/symbolic_executor/symbolic_executor.cpp
  core/target.cpp

  lib/collect_latent_statements.cpp
  lib/concolic.cpp
  lib/continuation.cpp
  lib/execution_state.cpp
  lib/final_state.cpp
  lib/gen_eq.cpp
  lib/logging.cpp
  lib/namespace_context.cpp
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

# Testgen libraries.
set(
  TESTGEN_LIBS
  p4tools-common
  inja
)

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
      set(compiler_targets_var "${compiler_targets_var}    ${ext}_registerCompilerTarget();\n")
      set(testgen_targets_var "${testgen_targets_var}    ${ext}_registerTestgenTarget();\n")
    endif()
  endif()
endforeach(ext)

# Propagate def files set by target extensions upwards.
set(IR_DEF_FILES ${IR_DEF_FILES} PARENT_SCOPE)

# Convert the list of files into #includes
foreach(include_file ${include_files})
endforeach()

# Fill the template
configure_file(register.h.in register.h)

add_p4tools_library(testgen ${TESTGEN_SOURCES})

target_link_libraries(
  testgen
  ${TESTGEN_LIBS}
)

add_p4tools_executable(p4testgen main.cpp)

target_link_libraries(
  p4testgen
  testgen
  ${TESTGEN_LIBS}
)

add_custom_target(
  linkp4testgen
  # Add some convenience links for invoking p4testgen.
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4testgen ${CMAKE_BINARY_DIR}/p4testgen
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
  )

  add_custom_target(
    linkgtest
    # Link P4 include files in a more convenient location.
    COMMAND
    for incl in p4include p4_14include \; do
    ${CMAKE_COMMAND} -E create_symlink
    ${P4C_BINARY_DIR}/\$$incl ${CMAKE_CURRENT_BINARY_DIR}/\$$incl \;
    done
  )
  add_dependencies(testgen-gtest linkgtest)

  if(ENABLE_TESTING)
    add_definitions("-DGTEST_HAS_PTHREAD=0")
    add_test(NAME testgen-gtest COMMAND testgen-gtest)
    set_tests_properties(testgen-gtest PROPERTIES LABELS "gtest-testgen")
  endif()

  if(ENABLE_IWYU)
    set_property(TARGET testgen-gtest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
  endif()
endif()

if(ENABLE_IWYU)
  set_property(TARGET testgen PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
