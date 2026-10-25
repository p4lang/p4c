# SPDX-FileCopyrightText: 2024 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

set(
  P4C_TOOLS_CONTROL_PLANE_SOURCES
  p4info_map.cpp
)

add_library(p4tools-control-plane STATIC ${P4C_TOOLS_CONTROL_PLANE_SOURCES})
target_link_libraries(p4tools-control-plane PUBLIC controlplane PRIVATE p4ctoolkit ir)
