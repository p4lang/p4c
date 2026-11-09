# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(P4RUNTIME_PROTO
  "${P4RUNTIME_STD_DIR}/google/rpc/status.proto"
  "${P4RUNTIME_STD_DIR}/p4/config/v1/p4info.proto"
  "${P4RUNTIME_STD_DIR}/p4/config/v1/p4types.proto"
  "${P4RUNTIME_STD_DIR}/p4/v1/p4data.proto"
  "${P4RUNTIME_STD_DIR}/p4/v1/p4runtime.proto"
)

# C++ bindings come from p4runtime::cpp. Keep Python bindings in the build's
# control-plane directory for the existing test drivers and tools.
protobuf_generate(
  LANGUAGE python
  PROTOS ${P4RUNTIME_PROTO}
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${Protobuf_INCLUDE_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  OUT_VAR P4RUNTIME_GEN_PY_SRCS
  DEPENDENCIES "${P4RUNTIME_PROTO};protobuf::protoc"
)
add_custom_target(controlplane-python ALL DEPENDS ${P4RUNTIME_GEN_PY_SRCS})

set(CONTROLPLANE_SRCS
  addMissingIds.cpp
  bytestrings.cpp
  flattenHeader.cpp
  p4infoApi.cpp
  p4RuntimeArchHandler.cpp
  p4RuntimeArchStandard.cpp
  p4RuntimeSerializer.cpp
  p4RuntimeSymbolTable.cpp
  typeSpecConverter.cpp
  bfruntime.cpp
)

set(CONTROLPLANE_HDRS
  addMissingIds.h
  bytestrings.h
  flattenHeader.h
  p4infoApi.h
  p4RuntimeArchHandler.h
  p4RuntimeArchStandard.h
  p4RuntimeSerializer.h
  p4RuntimeSymbolTable.h
  p4RuntimeTypes.h
  typeSpecConverter.h
  bfruntime.h
)

add_library(controlplane STATIC ${CONTROLPLANE_SRCS})
target_link_libraries(controlplane
  PUBLIC p4runtime::cpp
  # Required for the generated parser and the Abseil includes.
  PRIVATE frontend
  PRIVATE absl::strings
)

add_dependencies(controlplane controlplane-python)
