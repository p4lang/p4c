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

# Makefile for a fake backend that is used for testing the P4-16 front-end.

set (P4TEST_SRCS
  p4test.cpp
  midend.cpp
  )
set (P4TEST_HDRS
  midend.h
  )

add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${P4TEST_SRCS};${P4TEST_HDRS}")

build_unified(P4TEST_SRCS ALL)
add_executable(p4test ${P4TEST_SRCS} ${EXTENSION_P4_14_CONV_SOURCES})
target_link_libraries (p4test ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4test genIR)

install (TARGETS p4test
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkp4test
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4test ${P4C_BINARY_DIR}/p4test
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  )
add_dependencies(p4c_driver linkp4test)

# Tests

set(P4TEST_DRIVER ${P4C_SOURCE_DIR}/backends/p4test/run-p4-sample.py)

set (P4TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_errors/*.p4"
  )
set (P4_XFAIL_TESTS
  # issue #13
  testdata/p4_16_samples/cast-call.p4
  )
p4c_add_tests("p4" ${P4TEST_DRIVER} "${P4TEST_SUITES}" "${P4_XFAIL_TESTS}")

set (P4_14_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_errors/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/switch_*/switch.p4"
  )
p4c_add_tests("p14_to_16" ${P4TEST_DRIVER} "${P4_14_SUITES}" "")
