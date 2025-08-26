# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(P4RUNTIME_INFO_PROTO ${P4RUNTIME_STD_DIR}/p4/config/v1/p4info.proto)
set(P4RUNTIME_TYPES_PROTO ${P4RUNTIME_STD_DIR}/p4/config/v1/p4types.proto)
set(P4RUNTIME_RT_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4runtime.proto)
set(P4RUNTIME_DATA_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4data.proto)
set(P4RUNTIME_DEPS_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/google/rpc/status.proto)

set(P4RUNTIME_PROTO ${P4RUNTIME_DEPS_PROTO} ${P4RUNTIME_INFO_PROTO} ${P4RUNTIME_TYPES_PROTO}
                    ${P4RUNTIME_RT_PROTO} ${P4RUNTIME_DATA_PROTO}
)

# Generate source code from the .proto definitions using protoc. The output is placed in the build
# directory inside `control-plane`. For example, p4info.proto produces:
# control-plane/p4/config/p4info.pb.h control-plane/p4/config/p4info.pb.cc
add_custom_target(mkP4configdir ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/p4/config)

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

add_library(controlplane-gen OBJECT ${P4RUNTIME_PROTO})
target_include_directories(controlplane-gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(controlplane-gen PUBLIC protobuf::libprotobuf PRIVATE absl::log)
# Ideally we use DEPENDENCIES, but it only works in later versions (3.28).
add_dependencies(controlplane-gen mkP4configdir)
protobuf_generate(
  TARGET controlplane-gen
  LANGUAGE cpp
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${CMAKE_CURRENT_LIST_DIR} ${Protobuf_INCLUDE_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  OUT_VAR P4RUNTIME_GEN_CPP_SRCS
  DEPENDENCIES ${P4RUNTIME_PROTO} controlplane-gen mkP4configdir
)
protobuf_generate(
  TARGET controlplane-gen
  LANGUAGE python
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${CMAKE_CURRENT_LIST_DIR} ${Protobuf_INCLUDE_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  OUT_VAR P4RUNTIME_GEN_PY_SRCS
  DEPENDENCIES ${P4RUNTIME_PROTO} controlplane-gen mkP4configdir
)

# Silence various warnings as the root issue is out of our control, example
# https://github.com/protocolbuffers/protobuf/issues/7140
set_source_files_properties(${P4RUNTIME_GEN_CPP_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-array-bounds -Wno-error"
)

add_library(controlplane STATIC ${CONTROLPLANE_SRCS})
target_link_libraries(controlplane
  # Export Protobuf includes with controlplane-gen.
  PUBLIC controlplane-gen
  # Required for the generated parser and the Abseil includes.
  PRIVATE frontend
  PRIVATE absl::strings
)

add_dependencies(controlplane mkP4configdir)
