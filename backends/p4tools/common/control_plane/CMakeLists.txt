set(
  P4C_TOOLS_CONTROL_PLANE_SOURCES
  p4info_map.cpp
  symbolic_variables.cpp
)

add_p4tools_library(p4tools-control-plane ${P4C_TOOLS_CONTROL_PLANE_SOURCES})
target_link_libraries(p4tools-control-plane PRIVATE controlplane)
