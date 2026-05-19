# CMake file for ControlPlaneSmith.
project(rtsmith)

# Add cmake directory to our module path. Also add cmake directory from P4C
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# Declare common ControlPlaneSmith variables.
set(RTSMITH_DIR ${P4C_BINARY_DIR}/rtsmith)
set(RTSMITH_DRIVER "${CMAKE_CURRENT_BINARY_DIR}/p4rtsmith")
if (TARGET flay)
  set (FLAY_CHECKER_DRIVER ${CMAKE_CURRENT_BINARY_DIR}/tools/rtsmith_flay_checker)
endif()

# Source files for rtsmith.
set(RTSMITH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rtsmith.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/program_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/fuzzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/toml_utils.cpp
)

# GTest source files for ControlPlaneSmith.
set(RTSMITH_GTEST_SOURCES
   # # XXX These should be in a library.
  ${P4C_SOURCE_DIR}/test/gtest/helpers.cpp
  ${P4C_SOURCE_DIR}/test/gtest/gtestp4c.cpp
  test/core/rtsmith_api_test.cpp
  test/core/rtsmith_toml_test.cpp
)

# RTSmith libraries.

set(RTSMITH_LIBS p4tools-runtime-proto p4tools-common controlplane)

file(GLOB rtsmith_targets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/targets
     ${CMAKE_CURRENT_SOURCE_DIR}/targets/*
)
foreach(ext ${rtsmith_targets})
  set(rtsmith_targets_dir ${CMAKE_CURRENT_SOURCE_DIR}/targets/${ext}/)
  if(EXISTS ${rtsmith_targets_dir}/CMakeLists.txt AND IS_DIRECTORY ${rtsmith_targets_dir})
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
          "${include_statements_var}#include \"backends/p4tools/modules/rtsmith/targets/${ext}/register.h\"\n"
      )
      set(rtsmith_targets_var "${rtsmith_targets_var}    ${ext}_registerRtSmithTarget();\n")
    endif()
  endif()
endforeach(ext)

# Convert the list of files into #includes
foreach(include_file ${include_files})

endforeach()

# Fill the template
configure_file(register.h.in register.h)

add_library(rtsmith STATIC ${RTSMITH_SOURCES})
target_link_libraries(rtsmith ${RTSMITH_LIBS})

add_p4tools_executable(p4rtsmith main.cpp)

target_link_libraries(p4rtsmith rtsmith ${RTSMITH_LIBS})

# Integrate toml++ into the build.
include(FetchContent)
FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
target_link_libraries(rtsmith tomlplusplus::tomlplusplus)

# Define the path to the configuration file.
set(CONFIG_TOML_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test/configuration.toml)

# Copy the file to the binary directory.
add_custom_command(
  TARGET p4rtsmith POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CONFIG_TOML_PATH}
          $<TARGET_FILE_DIR:p4rtsmith>/test/configuration.toml
  COMMENT "Copying test/configuration.toml to the build directory."
)

# Add some convenience links for invoking rtsmith.
add_custom_target(
  linkrtsmith
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4rtsmith
          ${P4C_BINARY_DIR}/p4rtsmith
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include
          ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
)

add_dependencies(rtsmith linkrtsmith)

add_subdirectory(tools)

if(ENABLE_GTESTS)
  add_executable(rtsmith-gtest ${RTSMITH_GTEST_SOURCES})
  target_link_libraries(
    rtsmith-gtest PRIVATE rtsmith PRIVATE gtest ${RTSMITH_LIBS} ${P4C_LIBRARIES} ${P4C_LIB_DEPS}
  )

  if(ENABLE_TESTING)
    add_test(NAME rtsmith-gtest COMMAND rtsmith-gtest)
    set_tests_properties(rtsmith-gtest PROPERTIES LABELS "gtest-rtsmith")
  endif()

endif()
