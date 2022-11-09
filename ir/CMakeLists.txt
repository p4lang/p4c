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

set (IR_SRCS
  base.cpp
  dbprint.cpp
  dbprint-expression.cpp
  dbprint-stmt.cpp
  dbprint-type.cpp
  dbprint-p4.cpp
  dump.cpp
  expression.cpp
  ir.cpp
  irutils.cpp
  json_parser.cpp
  node.cpp
  pass_manager.cpp
  type.cpp
  v1.cpp
  visitor.cpp
  write_context.cpp
)

set (IR_HDRS
  configuration.h
  dbprint.h
  dump.h
  id.h
  indexed_vector.h
  ir-inline.h
  ir-tree-macros.h
  ir.h
  irutils.h
  json_generator.h
  json_loader.h
  json_parser.h
  namemap.h
  node.h
  nodemap.h
  pass_manager.h
  vector.h
  visitor.h
)

set (BASE_IR_DEF_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base.def
  ${CMAKE_CURRENT_SOURCE_DIR}/type.def
  ${CMAKE_CURRENT_SOURCE_DIR}/expression.def
  ${CMAKE_CURRENT_SOURCE_DIR}/ir.def
  ${CMAKE_CURRENT_SOURCE_DIR}/v1.def
)

set (IR_DEF_FILES ${IR_DEF_FILES} ${BASE_IR_DEF_FILES} PARENT_SCOPE)

build_unified(IR_SRCS)
add_library (ir STATIC ${IR_SRCS})
add_dependencies(ir genIR)


if(ENABLE_IWYU)
  message("Enabling IWYU checks.")
  find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
  set(
    iwyu_path
    ${iwyu_path}
    -Xiwyu
    --max_line_length=100
    -Xiwyu
    --no_fwd_decls
    -Xiwyu
    --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/p4c.imp
    -Xiwyu
    --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/boost-1.75-all.imp
    -Xiwyu
    --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/libcxx.imp
    -Xiwyu
    --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/gcc.libc.imp
    -Xiwyu
    --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/stl.c.headers.imp
  )
  message("IWYU command: ${iwyu_path}")
endif()

if(ENABLE_IWYU)
  set_property(TARGET ir PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
