# CMake file for common components of P4Tools.
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

include(common)

project(p4tools-common)

# Pull in a specific version of Z3 and link against it.
set(P4TOOLS_Z3_VERSION "4.11.2")
message("Fetching Z3 version ${P4TOOLS_Z3_VERSION} for P4Tools...")

# Print out download state while setting up Z3.
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)
fetchcontent_declare(
  z3
  URL https://github.com/Z3Prover/z3/releases/download/z3-${P4TOOLS_Z3_VERSION}/z3-${P4TOOLS_Z3_VERSION}-x64-glibc-2.31.zip
  URL_HASH SHA256=9d0f70e61e82b321f35e6cad1343615d2dead6f2c54337a24293725de2900fb6
  USES_TERMINAL_DOWNLOAD TRUE
  GIT_PROGRESS TRUE
)
fetchcontent_makeavailable(z3)
set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
message("Done with setting up Z3 for P4Tools.")


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

  core/abstract_execution_state.cpp
  core/target.cpp
  core/z3_solver.cpp

  lib/arch_spec.cpp
  lib/format_int.cpp
  lib/gen_eq.cpp
  lib/model.cpp
  lib/namespace_context.cpp
  lib/symbolic_env.cpp
  lib/table_utils.cpp
  lib/taint.cpp
  lib/trace_event.cpp
  lib/trace_event_types.cpp
  lib/util.cpp
  lib/variables.cpp
)

add_p4tools_library(p4tools-common ${P4C_TOOLS_COMMON_SOURCES})

# Other projects may also pull in Z3.
# We have to make sure we only include our local version with p4tools.
target_include_directories(p4tools-common SYSTEM BEFORE PUBLIC ${z3_SOURCE_DIR}/include)
set(P4TOOLS_Z3_LIB ${z3_SOURCE_DIR}/bin/libz3.a)

target_link_libraries(
  p4tools-common
  PUBLIC ${P4TOOLS_Z3_LIB}
)

target_include_directories(
  p4tools-common
  PUBLIC "${CMAKE_BINARY_DIR}/common"
  PUBLIC "${P4C_SOURCE_DIR}"
  PUBLIC "${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include"
  PUBLIC "${P4C_BINARY_DIR}"
)

add_dependencies(p4tools-common ir-generated frontend)
