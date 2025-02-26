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

if(NOT APPLE)
  # Fetch and declare the libbpf library. Print out download state while setting up libbpf.
  set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
  set(FETCHCONTENT_QUIET OFF)
  fetchcontent_declare(
    p4cbpfrepo
    URL https://github.com/libbpf/libbpf/archive/refs/tags/v1.5.0.tar.gz
    URL_HASH SHA256=53492aff6dd47e4da04ef5e672d753b9743848bdb38e9d90eafbe190b7983c44
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runtime/libbpf
    USES_TERMINAL_DOWNLOAD TRUE
    GIT_PROGRESS TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  fetchcontent_makeavailable(p4cbpfrepo)
  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  message("Building libbpf...")
  execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runtime/build-libbpf
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND_ECHO STDOUT
  )
  message("Done with setting up libbpf for P4C.")

  fetchcontent_declare(
    iproute2repo
    URL https://github.com/p4tc-dev/iproute2-p4tc-pub/archive/refs/tags/release-v17-rc6.tar.gz
    URL_HASH SHA256=624c32a571f9f30d1070d9b23e96121ac79f9273df9ff6db4ee6d034ab983c5d
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runtime/iproute2-p4tc-pub
    USES_TERMINAL_DOWNLOAD TRUE
    GIT_PROGRESS TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  fetchcontent_makeavailable(iproute2repo)
  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  message("Building iproute2...")
  execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runtime/build-iproute2 ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND_ECHO STDOUT
  )
  message("Done with setting up iproute2 for P4C.")
endif()

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

set(P4TC_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-tc-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4tc_samples/*.p4")

# Setup fixture
add_test(NAME p4tc_setup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/setup "https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest")
set_tests_properties(p4tc_setup PROPERTIES FIXTURES_SETUP P4TCFixture)

add_test(NAME p4tc_cleanup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/cleanup)
set_tests_properties(p4tc_cleanup PROPERTIES FIXTURES_CLEANUP P4TCFixture)

macro(p4tc_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  p4c_add_test_with_args(${tag} ${driver} ${isXfail} ${alias} ${p4test} ${test_args} "")
  p4c_test_set_name(__testname ${tag} ${alias})
  set_tests_properties(${__testname} PROPERTIES FIXTURES_REQUIRED P4TCFixture RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES TIMEOUT 1000)
endmacro(p4tc_add_test_with_args)

p4c_add_tests("p4tc" ${P4TC_COMPILER_DRIVER} "${P4_16_SUITES}" "")
p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/arp_respond.p4" "testdata/p4tc_samples_stf/arp_respond.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/arp_respond.stf" "")
p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/simple_l3.p4" "testdata/p4tc_samples_stf/simple_l3.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/simple_l3.stf" "")
