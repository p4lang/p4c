# Unity builds do not work for Protobuf...
set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
set(CMAKE_UNITY_BUILD OFF)
add_library(p4tools-runtime-proto OBJECT ${P4TOOLS_BFRUNTIME_RT_PROTO})
target_include_directories(p4tools-runtime-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(p4tools-runtime-proto PRIVATE controlplane)
protobuf_generate(
  TARGET p4tools-runtime-proto
  LANGUAGE cpp python
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${Protobuf_INCLUDE_DIRS} ${P4C_SOURCE_DIR}/control-plane ${P4C_SOURCE_DIR}
  PROTOC_OUT_DIR ${P4C_BINARY_DIR}
  OUT_VAR P4TOOLS_RUNTIME_PROTO_INFO_GEN_SRCS
)
# Reset temporary variable modifications.
set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})

set(
  P4C_TOOLS_CONTROL_PLANE_SOURCES
  p4info_map.cpp
  symbolic_variables.cpp
)

add_library(p4tools-control-plane STATIC ${P4C_TOOLS_CONTROL_PLANE_SOURCES})
target_link_libraries(p4tools-control-plane PRIVATE controlplane p4ctoolkit ir)
