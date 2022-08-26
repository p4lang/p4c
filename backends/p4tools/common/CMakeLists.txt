# CMake file for common components of P4Testgen.
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(p4testgen-common)

# We need a fairly recent version of Z3.
set(Z3_MIN_VERSION "4.8.14")
find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)

if(${Z3_VERSION} AND ${Z3_VERSION} VERSION_LESS ${Z3_MIN_VERSION})
  message(FATAL_ERROR "The minimum required Z3 version is ${Z3_MIN_VERSION}.")
endif()

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

# Source files for p4testgen-common.
set(
  P4C_TOOLS_COMMON_SOURCES
  options.cpp
  version.cpp

  compiler/compiler_target.cpp
  compiler/convert_errors.cpp
  compiler/convert_varbits.cpp
  compiler/copy_headers.cpp
  compiler/hs_index_simplify.cpp
  compiler/midend.cpp

  core/target.cpp
  core/z3_solver.cpp

  lib/coverage.cpp
  lib/format_bin_hex.cpp
  lib/formulae.cpp
  lib/ir.cpp
  lib/model.cpp
  lib/saturation_elim.cpp
  lib/symbolic_env.cpp
  lib/taint.cpp
  lib/timer.cpp
  lib/trace_events.cpp
  lib/util.cpp
  lib/zombie.cpp
)

add_p4tools_library(p4testgen-common ${P4C_TOOLS_COMMON_SOURCES})

target_link_libraries(
  p4testgen-common
  PUBLIC gsl-lite
  PUBLIC ${Boost_SYSTEM_LIBRARY}
  PUBLIC ${Boost_FILESYSTEM_LIBRARY}
  PUBLIC ${Z3_LIBRARY}
)

target_include_directories(
  p4testgen-common
  PUBLIC "${CMAKE_BINARY_DIR}/common"
  PUBLIC "${P4C_SOURCE_DIR}"
  PUBLIC "${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include"
  PUBLIC "${P4C_BINARY_DIR}"
)

add_dependencies(p4testgen-common genIR frontend)
if(ENABLE_IWYU)
  set_property(TARGET p4testgen-common PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
