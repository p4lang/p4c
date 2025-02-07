# Copyright (C) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#
# SPDX-License-Identifier: Apache-2.0

set (GEN_TOFINO
  memories.pipe_addrmap
  memories.pipe_top_level
  memories.prsr_mem_main_rspec
  regs.dprsr_hdr
  regs.dprsr_inp
  regs.ebp_rspec
  regs.ibp_rspec
  regs.mau_addrmap
  regs.pipe_addrmap
  regs.prsr_reg_merge_rspec
  regs.tofino
  )

foreach(f IN LISTS GEN_TOFINO)
  list (APPEND GEN_TOFINO_SRCS ${BFASM_BINARY_DIR}/gen/tofino/${f}.cpp)
  list (APPEND GEN_TOFINO_HDRS ${BFASM_BINARY_DIR}/gen/tofino/${f}.h)
endforeach()

set_source_files_properties(
  ${GEN_TOFINO_SRCS} ${GEN_TOFINO_HDRS}
  PROPERTIES GENERATED TRUE)

add_custom_command(OUTPUT ${GEN_TOFINO_HDRS} ${GEN_TOFINO_SRCS}
  COMMAND ${BFASM_WALLE} --schema chip.schema --generate-cpp template_objects.yaml -o ${BFASM_BINARY_DIR}/gen/tofino
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS template_objects.yaml chip.schema ${WALLE_SOURCES}
  COMMENT "Generating cpp code for tofino from tofino/chip.schema")

set (BFAS_TOFINO_SRCS
  tofino/exact_match.cpp
  tofino/gateway.cpp
  tofino/input_xbar.cpp
  tofino/parser.cpp
  tofino/sram_match.cpp
  tofino/stateful.cpp
  tofino/ternary_match.cpp
  PARENT_SCOPE
  )

set (BFAS_TOFINO_HEADERS
  tofino/action_table.h
  tofino/counter.h
  tofino/exact_match.h
  tofino/gateway.h
  tofino/input_xbar.h
  tofino/meter.h
  tofino/phv.h
  tofino/stateful.h
  tofino/ternary_match.h
  PARENT_SCOPE
  )

include_directories (${BFASM_BINARY_DIR}/gen/tofino)
add_library (regs_tofino ${GEN_TOFINO_SRCS} ${GEN_TOFINO_HDRS})
