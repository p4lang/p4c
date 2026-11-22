# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

# Makefile for a backend that generates code for the Behavioral Model version 2 (BMv2)
# compiling for the simple_switch target.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/simple_switch/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/simple_switch/version.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/psa_switch/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/psa_switch/version.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pna_nic/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/pna_nic/version.h" @ONLY)

# sources for backend executable
set (BMV2_SIMPLE_SWITCH_SRCS
    simple_switch/main.cpp
    simple_switch/midend.cpp
    simple_switch/midend.h
    simple_switch/simpleSwitch.cpp
    simple_switch/simpleSwitch.h
    simple_switch/options.h
    )
    
set (PORTABLE_COMMON
    portable_common/portable.cpp
    portable_common/portable.h  
    portable_common/options.cpp
    portable_common/options.h    
    portable_common/midend.cpp
    portable_common/midend.h
    )

set (BMV2_PSA_SWITCH_SRCS
    psa_switch/main.cpp
    psa_switch/midend.cpp
    psa_switch/midend.h
    psa_switch/psaSwitch.cpp
    psa_switch/psaSwitch.h
    psa_switch/options.cpp
    psa_switch/options.h
    )

set (BMV2_PNA_NIC_SRCS
    pna_nic/main.cpp
    pna_nic/midend.cpp
    pna_nic/midend.h
    pna_nic/pnaNic.cpp
    pna_nic/pnaNic.h
    pna_nic/pnaProgramStructure.cpp
    pna_nic/pnaProgramStructure.h
    pna_nic/options.cpp
    pna_nic/options.h
    )

set (BMV2_BACKEND_COMMON_SRCS
  common/JsonObjects.cpp
  common/action.cpp
  common/controlFlowGraph.cpp
  common/deparser.cpp
  common/expression.cpp
  common/extern.cpp
  common/globals.cpp
  common/header.cpp
  common/helpers.cpp
  common/lower.cpp
  common/parser.cpp
  )

set (BMV2_BACKEND_COMMON_HDRS
  common/JsonObjects.h
  common/action.h
  common/annotations.h
  common/backend.h
  common/check_unsupported.h
  common/control.h
  common/controlFlowGraph.h
  common/deparser.h
  common/expression.h
  common/extern.h
  common/globals.h
  common/header.h
  common/helpers.h
  common/lower.h
  common/midend.h
  common/options.h
  common/parser.h
  common/sharedActionSelectorCheck.h
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bmv2.def PARENT_SCOPE)

add_library(bmv2backend STATIC ${BMV2_BACKEND_COMMON_SRCS})
target_link_libraries(bmv2backend ir-generated frontend backends-common)

add_executable(p4c-bm2-ss ${BMV2_SIMPLE_SWITCH_SRCS})
target_link_libraries (p4c-bm2-ss bmv2backend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install(TARGETS p4c-bm2-ss RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_library(portable-common STATIC ${PORTABLE_COMMON})
target_link_libraries(portable-common bmv2backend)

add_executable(p4c-bm2-psa ${BMV2_PSA_SWITCH_SRCS})
target_link_libraries (p4c-bm2-psa portable-common ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
install(TARGETS p4c-bm2-psa RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_executable(p4c-bm2-pna ${BMV2_PNA_NIC_SRCS})
target_link_libraries (p4c-bm2-pna portable-common ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
install(TARGETS p4c-bm2-pna RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

file(RELATIVE_PATH
    CURRENT_BINARY_DIR_PATH_REL
    ${P4C_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(RELATIVE_PATH
    P4C_BINARY_DIR_PATH_REL
    ${CMAKE_CURRENT_BINARY_DIR}
    ${P4C_BINARY_DIR}
)

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkbmv2
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-ss ${P4C_BINARY_DIR}/p4c-bm2-ss
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-psa ${P4C_BINARY_DIR}/p4c-bm2-psa
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-pna ${P4C_BINARY_DIR}/p4c-bm2-pna
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkbmv2)

if (SUPPORT_P4_14)
add_custom_target(linkp414bmv2
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkp414bmv2)
endif()

set (GTEST_BMV2_SOURCES
  gtest/load_ir_from_json.cpp
)
set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_BMV2_SOURCES} PARENT_SCOPE)
set (GTEST_LDADD ${GTEST_LDADD} bmv2backend PARENT_SCOPE)
