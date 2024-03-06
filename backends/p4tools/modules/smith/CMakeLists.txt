# CMake file for P4Smith.

include(common)

project(smith)

# Declare common P4Testgen variables.
set(P4SMITH_DIR ${P4C_BINARY_DIR}/smith)
set(P4SMITH_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4smith")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY
)
# Source files for smith.
set(SMITH_SOURCES
    core/target.cpp
    backends/psa.cpp
    backends/structFieldList.cpp
    backends/top.cpp
    backends/tna.cpp
    backends/v1model.cpp
    common/declarations.cpp
    common/expressions.cpp
    common/p4state.cpp
    common/probabilities.cpp
    common/scope.cpp
    common/statements.cpp
    common/statementOrDeclaration.cpp
    common/table.cpp
    util/util.cpp
    util/wordlist.cpp
    options.cpp
    smith.cpp
)

set(SMITH_LIBS p4tools-common)

file(GLOB tools_targets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/targets
     ${CMAKE_CURRENT_SOURCE_DIR}/targets/*
)

foreach(ext ${tools_targets})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext}/CMakeLists.txt)
    # Generate an option that makes it possible to disable this extension.
    string(MAKE_C_IDENTIFIER ${ext} EXT_AS_IDENTIFIER)
    string(TOUPPER ${EXT_AS_IDENTIFIER} EXT_AS_OPTION_NAME)
    string(CONCAT ENABLE_EXT_OPTION "ENABLE_TOOLS_TARGET_" ${EXT_AS_OPTION_NAME})
    string(CONCAT EXT_HELP_TEXT "Build the " ${ext} " target")
    option(${ENABLE_EXT_OPTION} ${EXT_HELP_TEXT} ON)
    if(${ENABLE_EXT_OPTION})
      message("-- Enabling target ${ext}")
      add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext})
      set(include_statements_var
          "${include_statements_var}#include \"backends/p4tools/modules/smith/targets/${ext}/register.h\"\n"
      )
      set(compiler_targets_var "${compiler_targets}    ${ext}RegisterCompilerTarget();\n")
      set(smith_targets_var "${smith_targets}    ${ext}RegisterSmithTarget();\n")
    endif()
  endif()
endforeach(ext)

# Fill the template
configure_file(register.h.in register.h)

add_p4tools_library(smith ${SMITH_SOURCES})

add_dependencies(smith p4tools-common)

add_p4tools_executable(p4smith main.cpp)

target_link_libraries(
  p4smith PRIVATE smith ${SMITH_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target(
  linkp4smith # Add some convenience links for invoking p4smith.
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4smith
          ${CMAKE_BINARY_DIR}/p4smith
)

add_dependencies(p4smith linkp4smith)
