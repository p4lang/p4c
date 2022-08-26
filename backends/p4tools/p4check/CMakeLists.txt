# CMake file for Barefoot p4check.
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(p4check)

# Source files for p4check, excluding main.cpp.
set(
  P4CHECK_SOURCES
  commands.cpp
)

add_p4tools_executable(p4check main.cpp ${P4CHECK_SOURCES})

target_link_libraries(
  p4check
  PRIVATE p4testgen
  PRIVATE mutate
  PRIVATE smith
)

add_custom_target(
  linkp4check
  # Add some convenience links for invoking p4check.
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4check ${CMAKE_BINARY_DIR}/p4check
)

add_dependencies(linkp4check update_includes)
add_dependencies(p4check linkp4check)

if(ENABLE_IWYU)
  set_property(TARGET p4check PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
