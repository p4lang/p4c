# Handle the Z3 installation with this macro. Users have the option to supply their own Z3.
p4tools_obtain_z3()

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

target_link_libraries(
  p4tools-common
  PUBLIC ${P4TOOLS_Z3_LIB}
)

target_include_directories(
  p4tools-common
  SYSTEM BEFORE PUBLIC ${P4TOOLS_Z3_INCLUDE_DIR}
  PUBLIC "${CMAKE_BINARY_DIR}/common"
  PUBLIC "${P4C_SOURCE_DIR}"
  PUBLIC "${P4C_BINARY_DIR}"
)

add_dependencies(p4tools-common ir-generated frontend)
