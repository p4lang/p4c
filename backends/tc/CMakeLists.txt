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
    tcExterns.cpp
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
   tcExterns.h
   handleBitAlignment.h
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
target_link_libraries (p4c-pna-p4tc ${P4C_LIBRARIES} ${P4C_LIB_DEPS} backends-common)
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

# ### Tests ####

set(P4TC_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-tc-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4tc_samples/*.p4")

macro(p4tc_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  p4c_add_test_with_args(${tag} ${driver} ${isXfail} ${alias} ${p4test} ${test_args} "")
  p4c_test_set_name(__testname ${tag} ${alias})
  set_tests_properties(${__testname} PROPERTIES FIXTURES_REQUIRED P4TCFixture RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES TIMEOUT 1000)
endmacro(p4tc_add_test_with_args)

include(CheckLinuxKernel)
check_minimum_kernel_version("5.0.0" SUPPORTS_KERNEL)
check_minimum_linux_libc_version("5.0.0" SUPPORTS_LIBC)

set(ENABLE_P4TC_TESTS ON)
set(ENABLE_P4TC_STF_TESTS ON)

if (NOT SUPPORTS_KERNEL OR NOT SUPPORTS_LIBC)
  set(ENABLE_P4TC_TESTS OFF)
endif()

# A specific version of clang is required for the P4TC tests.
set (P4TC_CLANG "clang-15")
find_program(P4TC_CLANG_EXEC ${P4TC_CLANG})

if (NOT P4TC_CLANG_EXEC)
  set(ENABLE_P4TC_TESTS OFF)
endif()

# brctl is required for the P4TC STF tests.
find_program(BRCTL_EXEC brctl)
if(NOT BRCTL_EXEC)
  message(STATUS "brctl not found. Disabling P4TC STF tests.")
  set(ENABLE_P4TC_STF_TESTS OFF)
endif()

# Only enable P4TC and P4TC STF tests when all required tools are available.
if (ENABLE_P4TC_TESTS)
  p4c_add_tests("p4tc" ${P4TC_COMPILER_DRIVER} "${P4_16_SUITES}" "")

  if (ENABLE_P4TC_STF_TESTS)
    # Setup fixture
    add_test(NAME p4tc_setup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/setup "https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest")
    set_tests_properties(p4tc_setup PROPERTIES FIXTURES_SETUP P4TCFixture)

    add_test(NAME p4tc_cleanup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/cleanup)
    set_tests_properties(p4tc_cleanup PROPERTIES FIXTURES_CLEANUP P4TCFixture)

    p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/arp_respond.p4" "testdata/p4tc_samples_stf/arp_respond.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/arp_respond.stf" "")
    p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/simple_l3.p4" "testdata/p4tc_samples_stf/simple_l3.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/simple_l3.stf" "")
  endif()
endif()
