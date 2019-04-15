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

# The standard P4Runtime protocol buffers message definitions live in the PI
# repo, which is included in this repo as a submodule.
set (P4RUNTIME_STD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/p4runtime/proto)
set (P4RUNTIME_INFO_PROTO ${P4RUNTIME_STD_DIR}/p4/config/v1/p4info.proto)
set (P4RUNTIME_INFO_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/v1/p4info.pb.cc)
set (P4RUNTIME_INFO_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/v1/p4info.pb.h)

set (P4RUNTIME_TYPES_PROTO ${P4RUNTIME_STD_DIR}/p4/config/v1/p4types.proto)
set (P4RUNTIME_TYPES_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/v1/p4types.pb.cc)
set (P4RUNTIME_TYPES_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/v1/p4types.pb.h)

set (P4RUNTIME_RT_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4runtime.proto)
set (P4RUNTIME_RT_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/p4/v1/p4runtime.pb.cc)
set (P4RUNTIME_RT_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/p4/v1/p4runtime.pb.h)

set (P4RUNTIME_DATA_PROTO ${P4RUNTIME_STD_DIR}/p4/v1/p4data.proto)
set (P4RUNTIME_DATA_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/p4/v1/p4data.pb.cc)
set (P4RUNTIME_DATA_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/p4/v1/p4data.pb.h)

set (P4RUNTIME_DEPS_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/google/rpc/status.proto)
set (P4RUNTIME_DEPS_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.cc)
set (P4RUNTIME_DEPS_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/google/rpc/status.pb.h)

set (P4RUNTIME_GEN_SRCS ${P4RUNTIME_DEPS_GEN_SRCS}
                        ${P4RUNTIME_INFO_GEN_SRCS} ${P4RUNTIME_TYPES_GEN_SRCS}
                        ${P4RUNTIME_RT_GEN_SRCS} ${P4RUNTIME_DATA_GEN_SRCS})
set (P4RUNTIME_GEN_HDRS ${P4RUNTIME_DEPS_GEN_HDRS}
                        ${P4RUNTIME_INFO_GEN_HDRS} ${P4RUNTIME_TYPES_GEN_HDRS}
                        ${P4RUNTIME_RT_GEN_HDRS} ${P4RUNTIME_DATA_GEN_HDRS})
set (P4RUNTIME_PROTO ${P4RUNTIME_DEPS_PROTO}
                     ${P4RUNTIME_INFO_PROTO} ${P4RUNTIME_TYPES_PROTO}
                     ${P4RUNTIME_RT_PROTO} ${P4RUNTIME_DATA_PROTO})
set (P4RUNTIME_GEN_PYTHON "control-plane")

# Generate source code from the .proto definitions using protoc. The output is
# placed in the build directory inside `control-plane`. For example,
# p4info.proto produces:
#   control-plane/p4/config/p4info.pb.h
#   control-plane/p4/config/p4info.pb.cc
add_custom_target (mkP4configdir
  ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/p4/config)
add_custom_command(OUTPUT ${P4RUNTIME_GEN_SRCS} ${P4RUNTIME_GEN_HDRS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
          -I ${P4RUNTIME_STD_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}
          --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
          --python_out ${CMAKE_CURRENT_BINARY_DIR}
          ${P4RUNTIME_PROTO}
  DEPENDS ${P4RUNTIME_PROTO}
  COMMENT "Generating protobuf files"
  )

# These macros are much nicer than the custom command, but do not work for generating
# files in a different directory (e.g. p4/config). If we are ok with just generating
# the output in ${CMAKE_CURRENT_BINARY_DIR}, then these are fine. Also, for python there
# is no explicit dependency, so we need to make a target that always generates them (yuk!)
#PROTOBUF_GENERATE_CPP (P4RUNTIME_GEN_SRCS P4RUNTIME_INFO_HDRS ${P4RUNTIME_PROTO})
#PROTOBUF_GENERATE_PYTHON (P4RUNTIME_GEN_PYTHON P4RUNTIME_INFO_GEN_HDRS ${P4RUNTIME_INFO_PROTO})

set (CONTROLPLANE_SRCS
  flattenHeader.cpp
  p4RuntimeArchHandler.cpp
  p4RuntimeArchStandard.cpp
  p4RuntimeSerializer.cpp
  typeSpecConverter.cpp
  )
set (CONTROLPLANE_SOURCES
  ${P4RUNTIME_GEN_SRCS}
  ${CONTROLPLANE_SRCS}
  )

set (CONTROLPLANE_HDRS
  flattenHeader.h
  p4RuntimeArchHandler.h
  p4RuntimeArchStandard.h
  p4RuntimeSerializer.h
  typeSpecConverter.h
  )

# exclude p4RuntimeSerializer.cpp because it is has issues with
# include order. It would be nice to implement region-based NOLINT to
# handle this case.
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${CONTROLPLANE_HDRS}")

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties (${CONTROLPLANE_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
set_source_files_properties (${P4RUNTIME_GEN_SRCS} PROPERTIES GENERATED TRUE)
build_unified(CONTROLPLANE_SOURCES)
add_library (controlplane STATIC ${CONTROLPLANE_SOURCES} )
target_link_libraries (controlplane ${PROTOBUF_LIBRARY})
add_dependencies (controlplane mkP4configdir genIR frontend)
