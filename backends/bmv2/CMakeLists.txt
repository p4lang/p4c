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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/simple_switch/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/simple_switch/version.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/psa_switch/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/psa_switch/version.h" @ONLY)

# sources for backend executable
set (BMV2_SIMPLE_SWITCH_SRCS
    simple_switch/main.cpp
    simple_switch/midend.cpp
    simple_switch/midend.h
    simple_switch/simpleSwitch.cpp
    simple_switch/simpleSwitch.h
    )
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${BMV2_SIMPLE_SWITCH_SRCS}")

set (BMV2_PSA_SWITCH_SRCS
    psa_switch/main.cpp
    psa_switch/midend.cpp
    psa_switch/midend.h
    psa_switch/psaSwitch.cpp
    psa_switch/psaSwitch.h
    )
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${BMV2_PSA_SWITCH_SRCS}")

set (BMV2_BACKEND_COMMON_SRCS
  common/JsonObjects.cpp
  common/action.cpp
  common/control.cpp
  common/controlFlowGraph.cpp
  common/deparser.cpp
  common/expression.cpp
  common/extern.cpp
  common/globals.cpp
  common/header.cpp
  common/helpers.cpp
  common/lower.cpp
  common/metermap.cpp
  common/parser.cpp
  common/programStructure.cpp
  common/sharedActionSelectorCheck.cpp
  )

set (BMV2_BACKEND_COMMON_HDRS
  common/JsonObjects.h
  common/action.h
  common/annotations.h
  common/backend.h
  common/control.h
  common/controlFlowGraph.h
  common/deparser.h
  common/expression.h
  common/extern.h
  common/globals.h
  common/header.h
  common/helpers.h
  common/lower.h
  common/metermap.h
  common/midend.h
  common/options.h
  common/parser.h
  common/programStructure.h
  common/sharedActionSelectorCheck.h
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bmv2.def PARENT_SCOPE)
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${BMV2_BACKEND_COMMON_SRCS};${BMV2_BACKEND_COMMON_HDRS}")

build_unified(BMV2_BACKEND_COMMON_SRCS)
add_library(bmv2backend ${BMV2_BACKEND_COMMON_SRCS})
add_dependencies(bmv2backend genIR frontend)

build_unified(BMV2_SIMPLE_SWiTCH_SRCS)
add_executable(p4c-bm2-ss ${BMV2_SIMPLE_SWITCH_SRCS})
target_link_libraries (p4c-bm2-ss bmv2backend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install(TARGETS p4c-bm2-ss RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

build_unified(BMV2_PSA_SWiTCH_SRCS)
add_executable(p4c-bm2-psa ${BMV2_PSA_SWITCH_SRCS})
target_link_libraries (p4c-bm2-psa bmv2backend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
install(TARGETS p4c-bm2-psa RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# remove automake and fix the scripts.
add_custom_target(linkbmv2
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-bm2-ss ${P4C_BINARY_DIR}/p4c-bm2-ss
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-bm2-psa ${P4C_BINARY_DIR}/p4c-bm2-psa
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  )
add_dependencies(p4c_driver linkbmv2)


# Tests

set(BMV2_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/run-bmv2-test.py)

set (V1_INCLUDE_PATTERNS "include.*v1model.p4")
set (P4TESTS_FOR_V1MODEL "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4")
p4c_find_tests("${P4TESTS_FOR_V1MODEL}" v1tests INCLUDE "${V1_INCLUDE_PATTERNS}")

set (BMV2_V1MODEL_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_bmv_errors/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/switch_*/switch.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
  ${v1tests}
  )

set (PSA_INCLUDE_PATTERNS "include.*psa.p4")
set (PSA_EXCLUDE_PATTERNS "include.*v1model.p4")
set (P4TESTS_FOR_PSA "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4")
p4c_find_tests("${P4TESTS_FOR_PSA}" psa_tests INCLUDE "${PSA_INCLUDE_PATTERNS}" EXCLUDE "${PSA_EXCLUDE_PATTERNS}")

set (BMV2_PSA_TEST_SUITES
  ${psa_tests}
  )

# -p to use psa switch model
# -a to pass flags to compiler
set (testExtraFlags "${testExtraFlags} -p -a '--target bmv2 --arch psa'")

set (XFAIL_TESTS
  # This test defines two lpm keys for a table.
  # Even though the P4 spec allows it, it doesn't really make sense in a switch
  # so we allow it to fail on BMv2.
  testdata/p4_14_samples/issue60.p4
  # compiler claims (incorrectly?) that c2 has mulitple successors, so is not supported
  testdata/p4_14_samples/issue-1426.p4
  # As of 2019-Feb-04 latest p4c code, this program fails due to the
  # root cause of both issues #1694 and #1669.  I have tested it with
  # the proposed fix for issue #1694 that is on PR #1704, and while
  # that does make the produced BMv2 JSON file able to cause packets
  # to be recirculated, resubmitted, etc. it still has the bug of not
  # preserving the metadata specified in the program's field_lists, so
  # the STF tests fail because of issue #1669.  As of this writing,
  # this program is the _only_ p4c test program that relies for its
  # successful execution on the preservation of metadata across
  # resubmit, recirculate, and/or clone operations.
  testdata/p4_14_samples/p414-special-ops-2-bmv2.p4
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
  # These psa tests are not ready to run on bmv2 yet
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  testdata/p4_16_samples/psa-example-register2-bmv2.p4
  # This reads stack.next
  testdata/p4_16_samples/issue692-bmv2.p4
  # These test use computations in the verify/update checksum controls - unsupported
  testdata/p4_16_samples/issue1765-bmv2.p4
  testdata/p4_16_samples/issue1765-1-bmv2.p4
  # These tests require bmv2's psa_switch to support std metadata passing at minimum
  testdata/p4_16_samples/psa-ingress-input-meta-bmv2.p4
  # Next two use unknown externs
  testdata/p4_16_samples/issue1882-bmv2.p4
  testdata/p4_16_samples/issue1882-1-bmv2.p4
  # Uses non-constant array index
  testdata/p4_16_samples/issue1989-bmv2.p4
)

if (HAVE_SIMPLE_SWITCH)
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_V1MODEL_TEST_SUITES}" "${XFAIL_TESTS}")
  add_library(extern_func_module MODULE EXCLUDE_FROM_ALL "${P4C_SOURCE_DIR}/testdata/extern_modules/extern-funcs-bmv2.cpp" )
  p4c_add_test_with_args("bmv2" ${BMV2_DRIVER} FALSE "bmv2_emit_externs" "testdata/p4_16_samples/extern-funcs-bmv2.p4" "-a --emit-externs --target-specific-switch-arg \"--load-modules ${CMAKE_CURRENT_BINARY_DIR}/libextern_func_module.so\" --init \"make extern_func_module\"" "")
else()
  MESSAGE(WARNING "BMv2 simple switch is not available, not adding v1model BMv2 tests")
endif()

if (HAVE_PSA_SWITCH)
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_PSA_TEST_SUITES}" "${XFAIL_TESTS}" ${testExtraFlags})
else()
  MESSAGE(WARNING "BMv2 PSA switch is not available, not adding PSA BMv2 tests")
endif()

set (GTEST_BMV2_SOURCES
  )

set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_BMV2_SOURCES} PARENT_SCOPE)
set (GTEST_LDADD ${GTEST_LDADD} bmv2backend PARENT_SCOPE)
