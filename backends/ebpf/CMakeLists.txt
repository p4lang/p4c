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

# CMakefile for the EBPF P4-16 back-end.
message(STATUS "Start configuring the eBPF back end")

if(NOT APPLE)
  # Fetch and declare the libbpf library. Print out download state while setting up libbpf.
  set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
  set(FETCHCONTENT_QUIET OFF)
  fetchcontent_declare(
    bpfrepo
    URL https://github.com/libbpf/libbpf/archive/refs/tags/v1.2.2.tar.gz
    URL_HASH SHA256=32b0c41eabfbbe8e0c8aea784d7495387ff9171b5a338480a8fbaceb9da8d5e5
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/runtime/contrib/libbpf
    USES_TERMINAL_DOWNLOAD TRUE
    GIT_PROGRESS TRUE
  )
  fetchcontent_makeavailable(bpfrepo)
  set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  # Check if we have already built the libbpf library.
  find_library(LIBBPF NAMES bpf HINTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime/usr/lib64/")
  if (NOT LIBBPF)
    message("Building libbpf...")
    execute_process(
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_libbpf
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()
  message("Done with setting up libbpf for P4C.")
endif()



configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4C_EBPF_SRCS
  p4c-ebpf.cpp
  ebpfBackend.cpp
  ebpfProgram.cpp
  ebpfTable.cpp
  ebpfControl.cpp
  ebpfDeparser.cpp
  ebpfParser.cpp
  ebpfOptions.cpp
  target.cpp
  ebpfType.cpp
  codeGen.cpp
  ebpfModel.cpp
  midend.cpp
  lower.cpp
  ../bmv2/common/programStructure.cpp
  ../bmv2/psa_switch/psaProgramStructure.cpp
  psa/ebpfPsaGen.cpp
  psa/ebpfPipeline.cpp
  psa/ebpfPsaParser.cpp
  psa/ebpfPsaDeparser.cpp
  psa/ebpfPsaControl.cpp
  psa/ebpfPsaTable.cpp
  psa/backend.cpp
  psa/externs/ebpfPsaCounter.cpp
  psa/externs/ebpfPsaChecksum.cpp
  psa/externs/ebpfPsaDigest.cpp
  psa/externs/ebpfPsaHashAlgorithm.cpp
  psa/externs/ebpfPsaTableImplementation.cpp
  psa/externs/ebpfPsaRandom.cpp
  psa/externs/ebpfPsaRegister.cpp
  psa/externs/ebpfPsaMeter.cpp
)

set (P4C_EBPF_HDRS
  codeGen.h
  ebpfBackend.h
  ebpfControl.h
  ebpfDeparser.h
  ebpfModel.h
  ebpfObject.h
  ebpfProgram.h
  ebpfOptions.h
  ebpfParser.h
  ebpfTable.h
  ebpfType.h
  midend.h
  target.h
  lower.h
  ../bmv2/common/programStructure.h
  ../bmv2/psa_switch/psaProgramStructure.h
  ../bmv2/psa_switch/psaSwitch.h
  psa/backend.h
  psa/ebpfPsaGen.h
  psa/xdpHelpProgram.h
  psa/ebpfPipeline.h
  psa/ebpfPsaParser.h
  psa/ebpfPsaDeparser.h
  psa/ebpfPsaControl.h
  psa/ebpfPsaTable.h
  psa/externs/ebpfPsaCounter.h
  psa/externs/ebpfPsaChecksum.h
  psa/externs/ebpfPsaDigest.h
  psa/externs/ebpfPsaHashAlgorithm.h
  psa/externs/ebpfPsaTableImplementation.h
  psa/externs/ebpfPsaRegister.h
  psa/externs/ebpfPsaRandom.h
  psa/externs/ebpfPsaMeter.h
)

set (P4C_EBPF_DIST_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../p4include/ebpf_model.p4
  ${CMAKE_CURRENT_SOURCE_DIR}/../../p4include/xdp_model.p4
)

add_executable(p4c-ebpf ${P4C_EBPF_SRCS})
target_link_libraries (p4c-ebpf ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-ebpf ir-generated frontend)

install (TARGETS p4c-ebpf
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})

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
add_custom_target(linkp4cebpf
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-ebpf ${P4C_BINARY_DIR}/p4c-ebpf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
          ${CMAKE_COMMAND} -E copy ${P4C_EBPF_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  DEPENDS update_includes
  )

add_dependencies(p4c_driver linkp4cebpf)

# needs to be installed
# p4include_HEADERS += $(srcdir)/%reldir%/p4include/ebpf_model.p4

# Tests

set(EBPF_DRIVER_KERNEL "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t kernel -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")
set(EBPF_DRIVER_BCC "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t bcc -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")
set(EBPF_DRIVER_TEST "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t test -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")

set (XFAIL_TESTS_KERNEL)
set (XFAIL_TESTS_BCC
  # ternary not implemented for BCC
  ${P4C_SOURCE_DIR}/testdata/p4_16_samples/ternary_ebpf.p4
  )
set (XFAIL_TESTS_TEST
  # lpm and ternary not implemented for stf tests
  ${P4C_SOURCE_DIR}/testdata/p4_16_samples/lpm_ebpf.p4
  ${P4C_SOURCE_DIR}/testdata/p4_16_samples/ternary_ebpf.p4
  )

set (EBPF_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*_ebpf.p4"
  )
# Kernel tests for XDP won't work on other targets (e.g. missing bpf_xdp_* helpers).
set (EBPF_KERNEL_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*_ebpf-kernel.p4"
  )

# determine the kernel version
execute_process(COMMAND uname -r
  OUTPUT_VARIABLE P4C_EBPF_KERNEL_VER
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE rc)
message(STATUS "Detected kernel version: ${P4C_EBPF_KERNEL_VER}")
# Check if the kernel version is new enough to support ebpf features
set (MIN_KERNEL 4.15.0)
string (REGEX MATCH "[0-9]+[^-]*" KERNEL_VER ${CMAKE_SYSTEM})
if (${KERNEL_VER} VERSION_LESS ${MIN_KERNEL} )
  MESSAGE(WARNING "Kernel version ${KERNEL_VER} too small, expected ${MIN_KERNEL}. Ignoring ebpf kernel tests...")
  set (SUPPORTS_KERNEL False)
else()
  set (SUPPORTS_KERNEL True)
endif()

# Check if we have the right llvm version
set (MIN_LLVM 3.7.1)
# Grab the LLVM version, do not use find_package because it is unstable
# https://github.com/p4lang/p4c/issues/1376
set(LLVM_CMD "llvm-config --version")
message(STATUS "Check LLVM version with '${LLVM_CMD}'")
exec_program(${LLVM_CMD}
    RETURN_VALUE LLVM_RET
    OUTPUT_VARIABLE LLVM_PACKAGE_VERSION)
if (NOT LLVM_RET)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  if (${LLVM_PACKAGE_VERSION} VERSION_LESS ${MIN_LLVM})
    message(WARNING "LLVM version ${LLVM_PACKAGE_VERSION} too small, expected ${MIN_LLVM}.
    Ignoring ebpf tests...")
    set (SUPPORTS_KERNEL False)
  endif()
else()
  message(WARNING
    "Did not find an LLVM version that can compile the eBPF kernel tests...\n"
    "'llvm-config' reason: ${LLVM_PACKAGE_VERSION}\n"
    "'llvm-config' return value: ${LLVM_RET}" )
  set (SUPPORTS_KERNEL False)
endif()

# check for the libbpf library
find_library(LIBBPF NAMES bpf HINTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime/usr/lib64/")
if (LIBBPF)
  message(STATUS "Found libbpf library at ${LIBBPF}")
else()
  message(WARNING "Missing the libbpf dependency, disabling kernel tests."
  " You can install libbpf by running './build_libbpf' in the "
  "${CMAKE_CURRENT_SOURCE_DIR} folder.")
  set (SUPPORTS_KERNEL False)
endif()

# Only add the kernel tests if the two requirements are met
if (SUPPORTS_KERNEL)
  p4c_add_tests("ebpf-kernel" ${EBPF_DRIVER_KERNEL} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_KERNEL}")
  p4c_add_tests("ebpf-kernel" ${EBPF_DRIVER_KERNEL} ${EBPF_KERNEL_TEST_SUITES} "${XFAIL_TESTS_KERNEL}")
  # These are special tests with args that are not included
  # in the default ebpf tests
  p4c_add_test_with_args("ebpf-kernel" ${EBPF_DRIVER_KERNEL} FALSE "testdata/p4_16_samples/ebpf_conntrack_extern.p4" "testdata/p4_16_samples/ebpf_conntrack_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-conntrack-ebpf.c" "")
  p4c_add_test_with_args("ebpf-kernel" ${EBPF_DRIVER_KERNEL} FALSE "testdata/p4_16_samples/ebpf_checksum_extern.p4" "testdata/p4_16_samples/ebpf_checksum_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-checksum-ebpf.c" "")
endif()
# ToDo Add check which verifies that BCC is installed
# Ideally, this is done via check for the python package
p4c_add_tests("ebpf-bcc" ${EBPF_DRIVER_BCC} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_BCC}")
p4c_add_tests("ebpf" ${EBPF_DRIVER_TEST} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_TEST}")

# These are special tests with args that are not included in the default ebpf tests
p4c_add_test_with_args("ebpf" ${EBPF_DRIVER_TEST} FALSE "testdata/p4_16_samples/ebpf_checksum_extern.p4" "testdata/p4_16_samples/ebpf_checksum_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-checksum-ebpf.c" "")
# FIXME:This does not work yet
# We do not have support for dynamic addition of tables in the test framework
p4c_add_test_with_args("ebpf" ${EBPF_DRIVER_TEST} TRUE "testdata/p4_16_samples/ebpf_conntrack_extern.p4" "testdata/p4_16_samples/ebpf_conntrack_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-conntrack-ebpf.c" "")

message(STATUS "Done with configuring BPF back end")
