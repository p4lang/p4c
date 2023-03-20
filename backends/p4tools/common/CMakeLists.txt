# CMake file for common components of P4Tools.
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(p4tools-common)

# Boost filesystem is required for path handling.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system iostreams)

# Store the current git commit hash as part of version information.
execute_process(
  COMMAND git log --pretty=format:-%h -n 1
  OUTPUT_VARIABLE P4TOOLS_VERSION_HASH
  ERROR_QUIET
)

if(NOT "${P4TOOLS_VERSION_HASH}" STREQUAL "")
  message(STATUS "Current git refpoint: ${P4TOOLS_VERSION_HASH}")
endif()

# Generate version information.
configure_file(version.h.in version.h)

# Source files for p4tools-common.
set(
  P4C_TOOLS_COMMON_SOURCES
  options.cpp
  version.cpp

  compiler/compiler_target.cpp
  compiler/convert_hs_index.cpp
  compiler/convert_varbits.cpp
  compiler/midend.cpp
  compiler/reachability.cpp

  core/target.cpp
  core/z3_solver.cpp

  lib/format_int.cpp
  lib/formulae.cpp
  lib/model.cpp
  lib/symbolic_env.cpp
  lib/taint.cpp
  lib/trace_events.cpp
  lib/util.cpp
  lib/zombie.cpp
)

add_p4tools_library(p4tools-common ${P4C_TOOLS_COMMON_SOURCES})

target_link_libraries(
  p4tools-common
  PUBLIC gsl-lite
  PUBLIC ${Boost_SYSTEM_LIBRARY}
  PUBLIC ${Boost_FILESYSTEM_LIBRARY}
  PUBLIC ${Z3_LIBRARY}
)

target_include_directories(
  p4tools-common
  PUBLIC "${CMAKE_BINARY_DIR}/common"
  PUBLIC "${P4C_SOURCE_DIR}"
  PUBLIC "${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include"
  PUBLIC "${P4C_BINARY_DIR}"
)

add_dependencies(p4tools-common genIR frontend)
if(ENABLE_IWYU)
  set_property(TARGET p4tools-common PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
