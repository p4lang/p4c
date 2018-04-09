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

# Makefile for a backend that generates code for the Behavioral Model version 2 (BMv2)
# compiling for the simple_switch target.

# sources for backend executable
set (BMV2_SRCS bmv2.cpp)
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${BMV2_SRCS}")

set (BMV2_BACKEND_SRCS
  JsonObjects.cpp
  action.cpp
  analyzer.cpp
  backend.cpp
  control.cpp
  deparser.cpp
  errorcode.cpp
  expression.cpp
  extern.cpp
  extractArchInfo.cpp
  globals.cpp
  header.cpp
  helpers.cpp
  lower.cpp
  metermap.cpp
  midend.cpp
  parser.cpp
  portableSwitch.cpp
  sharedActionSelectorCheck.cpp
  simpleSwitch.cpp
  )

set (BMV2_BACKEND_HDRS
  JsonObjects.h
  action.h
  analyzer.h
  backend.h
  control.h
  deparser.h
  errorcode.h
  expression.h
  extern.h
  extractArchInfo.h
  globals.h
  header.h
  helpers.h
  lower.h
  metermap.h
  midend.h
  options.h
  parser.h
  portableSwitch.h
  sharedActionSelectorCheck.h
  simpleSwitch.h
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bmv2.def PARENT_SCOPE)
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${BMV2_BACKEND_SRCS}")

build_unified(BMV2_BACKEND_SRCS)
add_library(bmv2backend ${BMV2_BACKEND_SRCS})
add_dependencies(bmv2backend genIR)

build_unified(BMV2_SRCS)
add_executable(p4c-bm2-ss ${BMV2_SRCS})
target_link_libraries (p4c-bm2-ss bmv2backend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install(TARGETS p4c-bm2-ss RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkbmv2
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-bm2-ss ${P4C_BINARY_DIR}/p4c-bm2-ss
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  )
add_dependencies(p4c_driver linkbmv2)


# Tests

set(BMV2_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/run-bmv2-test.py)

set (BMV2_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_bmv_errors/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/switch_*/switch.p4"
  )

set (XFAIL_TESTS
  # This test defines two lpm keys for a table.
  # Even though the P4 spec allows it, it doesn't really make sense in a switch
  # so we allow it to fail on BMv2.
  testdata/p4_14_samples/issue60.p4
  # This test uses a feature currently unsupported in the BMv2 back-end.
  testdata/p4_16_samples/issue907-bmv2.p4
  # This test uses a table graph that is not implementable in BMv2
  testdata/p4_16_samples/issue986-bmv2.p4
  # These tests use a table key with type 'error'
  testdata/p4_16_samples/issue1062-bmv2.p4
  testdata/p4_16_samples/issue1062-1-bmv2.p4
  # This test uses an undefined extern
  testdata/p4_16_samples/issue1193-bmv2.p4
  # This test also uses a custom extern
  testdata/p4_14_samples/issue604.p4
  # This test uses an incorrect model
  testdata/p4_16_samples/issue1205-bmv2.p4
  )

if (HAVE_SIMPLE_SWITCH)
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_TEST_SUITES}" "${XFAIL_TESTS}")
else()
  MESSAGE(WARNING "BMv2 simple switch is not available, not adding BMv2 tests")
endif()

set (GTEST_BMV2_SOURCES
  ${P4C_SOURCE_DIR}/test/gtest/bmv2_isvalid.cpp
  )

set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_BMV2_SOURCES} PARENT_SCOPE)
set (GTEST_LDADD ${GTEST_LDADD} bmv2backend PARENT_SCOPE)
