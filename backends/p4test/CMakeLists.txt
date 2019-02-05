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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

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
add_dependencies(p4test genIR frontend)

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
# Builds a list of tests for which P4Info generation is not supported. It makes
# sense not to fail the test just because P4Info generation fails. It probably
# would not make sense to run all the tests twice (one with and one without
# P4Runtime).
set (P4RUNTIME_EXCLUDE
  # Two counter instances provided for psa_direct_counter, illegal according to
  # PSA spec and fails in P4Info generation
  testdata/p4_16_samples/psa-counter5.p4
  # Need to convert PSA_MeterColor_t to a serializable enum
  testdata/p4_16_samples/psa-meter1.p4
  # error not supported in static entries, not strictly required for P4Info
  # generation though
  testdata/p4_16_samples/issue1062-bmv2.p4
)
# Do not generate P4Info for tests which are expected to fail P4 compilation.
p4c_find_test_names ("${P4C_SOURCE_DIR}/testdata/p4_16_errors/*.p4" P4RUNTIME_EXCLUDE_ERRORS)
list (APPEND P4RUNTIME_EXCLUDE ${P4RUNTIME_EXCLUDE_ERRORS})
set (P4_XFAIL_TESTS
  # issue #13
  testdata/p4_16_samples/cast-call.p4
  )
p4c_add_tests_w_p4runtime("p4" ${P4TEST_DRIVER} "${P4TEST_SUITES}" "${P4_XFAIL_TESTS}" "${P4RUNTIME_EXCLUDE}")

set (P4_14_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/switch_*/switch.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_errors/*.p4"
  )
p4c_add_tests("p14_to_16" ${P4TEST_DRIVER} "${P4_14_SUITES}" "")
