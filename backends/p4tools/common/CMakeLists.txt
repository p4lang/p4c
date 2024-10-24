# Handle the Z3 installation with this macro. Users have the option to supply their own Z3.
include(${P4C_SOURCE_DIR}/cmake/Z3.cmake)
p4tools_obtain_z3()

# Generate version information.
configure_file(version.h.in version.h)

# Source files for p4tools-common.
set(
  P4C_TOOLS_COMMON_SOURCES
  options.cpp
  version.cpp

  compiler/compiler_result.cpp
  compiler/compiler_target.cpp
  compiler/convert_hs_index.cpp
  compiler/convert_struct_expr.cpp
  compiler/convert_varbits.cpp
  compiler/midend.cpp
  compiler/reachability.cpp

  core/abstract_execution_state.cpp
  core/target.cpp
  core/z3_solver.cpp

  lib/arch_spec.cpp
  lib/format_int.cpp
  lib/gen_eq.cpp
  lib/logging.cpp
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

add_library(p4tools-common OBJECT ${P4C_TOOLS_COMMON_SOURCES})

target_link_libraries(
  p4tools-common
  # We export Z3' with the common library.
  PUBLIC ${P4TOOLS_Z3_LIB}
    # For Abseil includes.
  PRIVATE frontend
)

target_include_directories(
  p4tools-common
  # We export Z3's includes with the common library.
  SYSTEM BEFORE PUBLIC ${P4TOOLS_Z3_INCLUDE_DIR}
)

# Add control-plane-specific extensions.
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/control_plane)
