#******************************************************************************
# Copyright (C) 2023 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions
# and limitations under the License.
#*****************************************************************************/

set(P4TC_BACKEND_SOURCES
    backend.cpp
    ebpfCodeGen.cpp
    introspection.cpp
    midend.cpp
    pnaProgramStructure.cpp
    tc.cpp
    tcAnnotations.cpp
    version.cpp
    ../ebpf/ebpfBackend.cpp
    ../ebpf/ebpfProgram.cpp
    ../ebpf/ebpfTable.cpp
    ../ebpf/ebpfControl.cpp
    ../ebpf/ebpfDeparser.cpp
    ../ebpf/ebpfParser.cpp
    ../ebpf/ebpfOptions.cpp
    ../ebpf/target.cpp
    ../ebpf/ebpfType.cpp
    ../ebpf/codeGen.cpp
    ../ebpf/ebpfModel.cpp
    ../ebpf/midend.cpp
    ../ebpf/lower.cpp
    ../bmv2/common/programStructure.cpp
    ../bmv2/psa_switch/psaProgramStructure.cpp
    ../ebpf/psa/ebpfPsaGen.cpp
    ../ebpf/psa/ebpfPipeline.cpp
    ../ebpf/psa/ebpfPsaParser.cpp
    ../ebpf/psa/ebpfPsaDeparser.cpp
    ../ebpf/psa/ebpfPsaControl.cpp
    ../ebpf/psa/ebpfPsaTable.cpp
    ../ebpf/psa/backend.cpp
    ../ebpf/psa/externs/ebpfPsaCounter.cpp
    ../ebpf/psa/externs/ebpfPsaChecksum.cpp
    ../ebpf/psa/externs/ebpfPsaDigest.cpp
    ../ebpf/psa/externs/ebpfPsaHashAlgorithm.cpp
    ../ebpf/psa/externs/ebpfPsaTableImplementation.cpp
    ../ebpf/psa/externs/ebpfPsaRandom.cpp
    ../ebpf/psa/externs/ebpfPsaRegister.cpp
    ../ebpf/psa/externs/ebpfPsaMeter.cpp
    )

set(P4TC_BACKEND_HEADERS
   backend.h
   ebpfCodeGen.h
   introspection.h
   midend.h
   options.h
   pnaProgramStructure.h
   tcAnnotations.h
   version.h
   ../ebpf/codeGen.h
   ../ebpf/ebpfBackend.h
   ../ebpf/ebpfControl.h
   ../ebpf/ebpfDeparser.h
   ../ebpf/ebpfModel.h
   ../ebpf/ebpfObject.h
   ../ebpf/ebpfProgram.h
   ../ebpf/ebpfOptions.h
   ../ebpf/ebpfParser.h
   ../ebpf/ebpfTable.h
   ../ebpf/ebpfType.h
   ../ebpf/midend.h
   ../ebpf/target.h
   ../ebpf/lower.h
   ../bmv2/common/programStructure.h
   ../bmv2/psa_switch/psaProgramStructure.h
   ../bmv2/psa_switch/psaSwitch.h
   ../ebpf/psa/backend.h
   ../ebpf/psa/ebpfPsaGen.h
   ../ebpf/psa/xdpHelpProgram.h
   ../ebpf/psa/ebpfPipeline.h
   ../ebpf/psa/ebpfPsaParser.h
   ../ebpf/psa/ebpfPsaDeparser.h
   ../ebpf/psa/ebpfPsaControl.h
   ../ebpf/psa/ebpfPsaTable.h
   ../ebpf/psa/externs/ebpfPsaCounter.h
   ../ebpf/psa/externs/ebpfPsaChecksum.h
   ../ebpf/psa/externs/ebpfPsaDigest.h
   ../ebpf/psa/externs/ebpfPsaHashAlgorithm.h
   ../ebpf/psa/externs/ebpfPsaTableImplementation.h
   ../ebpf/psa/externs/ebpfPsaRegister.h
   ../ebpf/psa/externs/ebpfPsaRandom.h
   ../ebpf/psa/externs/ebpfPsaMeter.h
   )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/tc.def PARENT_SCOPE)

# build
add_executable(p4c-pna-p4tc ${P4TC_BACKEND_SOURCES})
target_link_libraries (p4c-pna-p4tc ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-pna-p4tc genIR)

# install
install (TARGETS p4c-pna-p4tc
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(linkp4tc
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-pna-p4tc ${P4C_BINARY_DIR}/p4c-pna-p4tc
        )

add_dependencies(p4c_driver linkp4tc)

add_custom_target(tc_includes ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include/tc
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${P4C_SOURCE_DIR}/p4include/tc/pna.p4 ${P4C_BINARY_DIR}/p4include/tc)

set(P4TC_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-tc-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4tc_samples/*.p4")

p4c_add_tests("p4tc" ${P4TC_COMPILER_DRIVER} "${P4_16_SUITES}" "")
