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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

################ Proto
set (DPDK_P4RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/control-plane/proto)
set (DPDK_P4RUNTIME_INFO_PROTO ${DPDK_P4RUNTIME_DIR}/p4info.proto)
set (DPDK_P4RUNTIME_INFO_GEN_SRCS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/p4info.pb.cc)
set (DPDK_P4RUNTIME_INFO_GEN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/p4/config/p4info.pb.h)

add_custom_target (dpdk_runtime_dir
  ${CMAKE_COMMAND} -E make_directory
  ${CMAKE_CURRENT_BINARY_DIR}/p4/config)
# Generate source code from .proto using protoc. The output is
# placed in the build directory inside `control-plane` directory
add_custom_command(
    OUTPUT ${DPDK_P4RUNTIME_INFO_GEN_SRCS} ${DPDK_P4RUNTIME_INFO_GEN_HDRS}
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    --proto_path ${DPDK_P4RUNTIME_DIR}
    --proto_path ${P4RUNTIME_STD_DIR}
    --cpp_out ${CMAKE_CURRENT_BINARY_DIR}/p4/config
    --python_out ${CMAKE_CURRENT_BINARY_DIR}/p4/config
          ${DPDK_P4RUNTIME_INFO_PROTO}
  DEPENDS ${DPDK_P4RUNTIME_INFO_PROTO} dpdk_runtime_dir
  COMMENT "Generating protobuf files for p4info on target DPDK."
  )
add_dependencies(dpdk_runtime_dir controlplane)

add_library(dpdk_runtime STATIC ${DPDK_P4RUNTIME_INFO_GEN_SRCS})
target_include_directories(dpdk_runtime
  SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
)

# Silence various warnings as the root issue is out of our control, example https://github.com/protocolbuffers/protobuf/issues/7140
set_source_files_properties(${DPDK_P4RUNTIME_INFO_GEN_SRCS} {$DPDK_P4RUNTIME_INFO_GEN_HDRS} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-array-bounds -Wno-error")

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
target_link_libraries (p4c-dpdk dpdk_runtime ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-dpdk dpdk_runtime frontend)

install (TARGETS p4c-dpdk
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(linkp4cdpdk
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-dpdk ${P4C_BINARY_DIR}/p4c-dpdk
        DEPENDS update_includes
)

add_dependencies(p4c_driver linkp4cdpdk)

# Tests
set(DPDK_PTF_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-dpdk-ptf-test.py")
set(DPDK_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-dpdk-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/psa-*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pna-*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_psa_errors/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_dpdk_errors/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_pna_errors/*.p4")
 p4c_add_tests("dpdk" ${DPDK_COMPILER_DRIVER} "${P4_16_SUITES}" "" "--bfrt")

#### DPDK-PTF Tests
# PTF tests for DPDK are only enabled when both infrap4d and dpdk-target are installed.
set(DPDK_PTF_TEST_SUITES "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pna-dpdk-add_on_miss0.p4")
# Check for infrap4d.
find_program(INFRAP4D infrap4d PATHS ${IPDK_INSTALL_DIR}/sbin)
# DPDK_SDK_INSTALL is the path to the dpdk-target install directory
if(INFRAP4D AND EXISTS ${IPDK_INSTALL_DIR})
  set (DPDK_LIB_PATH "${IPDK_INSTALL_DIR}/lib;${IPDK_INSTALL_DIR}/lib/x86_64-linux-gnu")
  p4c_add_tests(
    "dpdk-ptf" ${DPDK_PTF_DRIVER} "${DPDK_PTF_TEST_SUITES}" ""
    "--ipdk-install-dir=${IPDK_INSTALL_DIR} -ll=DEBUG "
  )
else()
  message(WARNING "Either infrap4d or dpdk-target is not available (required environment variables are not defined). Skipping DPDK PTF tests")
endif()

include(DpdkXfail.cmake)
