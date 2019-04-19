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

# Makefile for the EBPF P4-16 back-end.
# To be included in the main P4C compiler makefile

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4C_EBPF_SRCS
  p4c-ebpf.cpp
  ebpfBackend.cpp
  ebpfProgram.cpp
  ebpfTable.cpp
  ebpfControl.cpp
  ebpfParser.cpp
  target.cpp
  ebpfType.cpp
  codeGen.cpp
  ebpfModel.cpp
  midend.cpp
  lower.cpp
  )

set (P4C_EBPF_HDRS
  codeGen.h
  ebpfBackend.h
  ebpfControl.h
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
  )

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${P4C_EBPF_SRCS};${P4C_EBPF_HDRS}")

set (P4C_EBPF_DIST_HEADERS p4include/ebpf_model.p4)

build_unified(P4C_EBPF_SRCS ALL)
add_executable(p4c-ebpf ${P4C_EBPF_SRCS})
target_link_libraries (p4c-ebpf ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-ebpf genIR frontend)

install (TARGETS p4c-ebpf
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkp4cebpf
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-ebpf ${P4C_BINARY_DIR}/p4c-ebpf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
          ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${P4C_EBPF_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  )

add_dependencies(p4c_driver linkp4cebpf)

# needs to be installed
# p4include_HEADERS += $(srcdir)/%reldir%/p4include/ebpf_model.p4

# Tests

set(EBPF_DRIVER_KERNEL "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t kernel -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")
set(EBPF_DRIVER_BCC "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t bcc -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")
set(EBPF_DRIVER_TEST "${CMAKE_CURRENT_SOURCE_DIR}/run-ebpf-test.py -t test -c \"${P4C_BINARY_DIR}/p4c-ebpf\"")

set (XFAIL_TESTS_KERNEL)
set (XFAIL_TESTS_BCC)
set (XFAIL_TESTS_TEST)

set (EBPF_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*_ebpf.p4"
  )

set (SUPPORTS_KERNEL True)

# Check if the kernel version is new enough to support ebpf features
set (MIN_KERNEL 4.15.0)
string (REGEX MATCH "[0-9]+[^-]*" KERNEL_VER ${CMAKE_SYSTEM})
if (${KERNEL_VER} VERSION_LESS ${MIN_KERNEL} )
  MESSAGE(WARNING "Kernel version ${KERNEL_VER} too small, expected ${MIN_KERNEL}. Ignoring ebpf kernel tests...")
  set (SUPPORTS_KERNEL False)
endif()

# Check if we have the right llvm version
set (MIN_LLVM 3.7.1)

# If you get an error here see
# https://stackoverflow.com/questions/38171543/error-when-using-cmake-with-llvm
find_package(LLVM QUIET)
if (LLVM_PACKAGE_VERSION)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  if (${LLVM_PACKAGE_VERSION} VERSION_LESS ${MIN_LLVM})
    MESSAGE(WARNING "LLVM version ${LLVM_PACKAGE_VERSION} too small, expected ${MIN_LLVM}.
    Ignoring ebpf tests...")
    set (SUPPORTS_KERNEL False)
  endif()
else()
  message(STATUS "LLVM missing, disabling kernel tests...")
  set (SUPPORTS_KERNEL False)
endif()

# Only add the kernel tests if the two requirements are met
if (SUPPORTS_KERNEL)
  p4c_add_tests("ebpf-kernel" ${EBPF_DRIVER_KERNEL} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_KERNEL}")
endif()
# ToDo Add check which verifies that BCC is installed
# Ideally, this is done via check for the python package
p4c_add_tests("ebpf-bcc" ${EBPF_DRIVER_BCC} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_BCC}")
p4c_add_tests("ebpf" ${EBPF_DRIVER_TEST} ${EBPF_TEST_SUITES} "${XFAIL_TESTS_TEST}")
