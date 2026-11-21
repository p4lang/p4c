# Copyright 2020 Intel Corp.
# SPDX-FileCopyrightText: 2020 Intel Corp.
#
# SPDX-License-Identifier: Apache-2.0

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

################ Proto
set (DPDK_P4RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/control-plane/proto)
set (DPDK_P4RUNTIME_INFO_PROTO ${DPDK_P4RUNTIME_DIR}/p4info.proto)

add_custom_target (dpdk_runtime_dir
  ${CMAKE_COMMAND} -E make_directory
  ${CMAKE_CURRENT_BINARY_DIR}/p4/config)

# Generate source code from .proto using protoc. The output is
# placed in the build directory inside `control-plane` directory
add_library(dpdk_runtime OBJECT ${DPDK_P4RUNTIME_INFO_PROTO})
target_include_directories(dpdk_runtime PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(dpdk_runtime PUBLIC protobuf::libprotobuf)
# Ideally we use DEPENDENCIES, but it only works in later versions (3.28).
add_dependencies(dpdk_runtime dpdk_runtime_dir)
protobuf_generate(
  TARGET dpdk_runtime
  LANGUAGE cpp
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${DPDK_P4RUNTIME_DIR} ${Protobuf_INCLUDE_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/p4/config
  OUT_VAR DPDK_P4RUNTIME_INFO_CPP_GEN_SRCS
  DEPENDENCIES ${DPDK_P4RUNTIME_INFO_PROTO} dpdk_runtime_dir
)
protobuf_generate(
  TARGET dpdk_runtime
  LANGUAGE python
  IMPORT_DIRS ${P4RUNTIME_STD_DIR} ${DPDK_P4RUNTIME_DIR} ${Protobuf_INCLUDE_DIRS}
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/p4/config
  OUT_VAR DPDK_P4RUNTIME_INFO_PY_GEN_SRCS
  DEPENDENCIES ${DPDK_P4RUNTIME_INFO_PROTO} dpdk_runtime_dir
)

# Silence various warnings as the root issue is out of our control, example https://github.com/protocolbuffers/protobuf/issues/7140
set_source_files_properties(${DPDK_P4RUNTIME_INFO_CPP_GEN_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-array-bounds -Wno-error")

set(P4C_DPDK_SOURCES
    ../bmv2/common/lower.cpp
    backend.cpp
    main.cpp
    midend.cpp
    dpdkHelpers.cpp
    dpdkProgram.cpp
    dpdkProgramStructure.cpp
    dpdkArch.cpp
    dpdkContext.cpp
    dpdkAsmOpt.cpp
    dpdkMetadata.cpp
    dpdkUtils.cpp
    options.cpp
    control-plane/bfruntime_ext.cpp
    tdiConf.cpp
)

set(P4C_DPDK_HEADERS
    ../bmv2/common/lower.h
    backend.h
    midend.h
    dpdkCheckExternInvocation.h
    dpdkHelpers.h
    dpdkProgram.h
    dpdkArch.h
    dpdkContext.h
    constants.h
    dpdkAsmOpt.h
    dpdkMetadata.h
    printUtils.h
    dpdkUtils.h
    dpdkProgramStructure.h
    options.h
    control-plane/bfruntime_ext.h
    control-plane/bfruntime_arch_handler.h
    tdiConf.h
)

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/dpdk.def PARENT_SCOPE)

set(DPDK_IR_SRCS
    spec.cpp
    printUtils.cpp
    dbprint-dpdk.cpp
    )
# IR sources that need to go into the frontend library
set(QUAL_DPDK_IR_SRCS)
foreach(src IN LISTS DPDK_IR_SRCS)
    set(QUAL_DPDK_IR_SRCS ${QUAL_DPDK_IR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(EXTENSION_IR_SOURCES ${EXTENSION_IR_SOURCES} ${QUAL_DPDK_IR_SRCS} PARENT_SCOPE)

add_executable(p4c-dpdk ${P4C_DPDK_SOURCES})
target_link_libraries (p4c-dpdk dpdk_runtime frontend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install (TARGETS p4c-dpdk
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(linkp4cdpdk
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-dpdk ${P4C_BINARY_DIR}/p4c-dpdk
        DEPENDS update_includes
)

add_dependencies(p4c_driver linkp4cdpdk)
