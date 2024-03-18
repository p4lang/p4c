set(
  P4C_TOOLS_CONTROL_PLANE_SOURCES
  p4info_map.cpp
  symbolic_variables.cpp
)

add_library(p4tools-control-plane STATIC ${P4C_TOOLS_CONTROL_PLANE_SOURCES})
target_link_libraries(p4tools-control-plane PRIVATE controlplane)
