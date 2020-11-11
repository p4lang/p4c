# Copyright 2020 Intel Corp.
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

set(P4C_DPDK_SOURCES
    ../bmv2/common/helpers.cpp
    ../bmv2/common/JsonObjects.cpp
    ../bmv2/common/action.cpp
    ../bmv2/common/controlFlowGraph.cpp
    ../bmv2/common/deparser.cpp
    ../bmv2/common/expression.cpp
    ../bmv2/common/extern.cpp
    ../bmv2/common/globals.cpp
    ../bmv2/common/header.cpp
    ../bmv2/common/helpers.cpp
    ../bmv2/common/lower.cpp
    ../bmv2/common/metermap.cpp
    ../bmv2/common/parser.cpp
    ../bmv2/common/programStructure.cpp
    ../bmv2/psa_switch/psaSwitch.cpp
    backend.cpp
    main.cpp
    midend.cpp
    dpdkHelpers.cpp
    dpdkProgram.cpp
    dpdkVarCollector.cpp
    dpdkArch.cpp
    dpdkAsmOpt.cpp
    elimTypedef.cpp
    )

set(P4C_DPDK_HEADERS
    ../bmv2/psa_switch/midend.h
    ../bmv2/common/helpers.h
    ../bmv2/common/JsonObjects.h
    ../bmv2/common/lower.h
    ../bmv2/common/action.h
    ../bmv2/common/annotations.h
    ../bmv2/common/backend.h
    ../bmv2/common/control.h
    ../bmv2/common/controlFlowGraph.h
    ../bmv2/common/deparser.h
    ../bmv2/common/expression.h
    ../bmv2/common/extern.h
    ../bmv2/common/globals.h
    ../bmv2/common/header.h
    ../bmv2/common/helpers.h
    ../bmv2/common/lower.h
    ../bmv2/common/metermap.h
    ../bmv2/common/midend.h
    ../bmv2/common/options.h
    ../bmv2/common/parser.h
    ../bmv2/common/programStructure.h
    ../bmv2/common/sharedActionSelectorCheck.h
    ../bmv2/psa_switch/psaSwitch.h
    backend.h
    midend.h
    dpdkHelpers.h
    dpdkProgram.h
    dpdkVarCollector.h
    dpdkArch.h
    dpdkAsmOpt.h
    elimTypedef.h
    options.h
    )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/dpdk.def PARENT_SCOPE)

set(DPDK_IR_SRCS
    spec.cpp
    )
# IR sources that need to go into the frontend library
set(QUAL_DPDK_IR_SRCS)
foreach(src IN LISTS DPDK_IR_SRCS)
    set(QUAL_DPDK_IR_SRCS ${QUAL_DPDK_IR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(EXTENSION_FRONTEND_SOURCES ${EXTENSION_FRONTEND_SOURCES} ${QUAL_DPDK_IR_SRCS} PARENT_SCOPE)

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${P4C_DPDK_SOURCES}" "${P4C_DPDK_HEADERS}" "${DPDK_IR_SRCS}")
add_executable(p4c-dpdk ${P4C_DPDK_SOURCES})
target_link_libraries (p4c-dpdk ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-dpdk genIR frontend)

install (TARGETS p4c-dpdk
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(linkp4cdpdk
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-dpdk ${P4C_BINARY_DIR}/p4c-dpdk
        )

add_dependencies(p4c_driver linkp4cdpdk)

