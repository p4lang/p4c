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
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pna_nic/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/pna_nic/version.h" @ONLY)

# sources for backend executable
set (BMV2_SIMPLE_SWITCH_SRCS
    simple_switch/main.cpp
    simple_switch/midend.cpp
    simple_switch/midend.h
    simple_switch/simpleSwitch.cpp
    simple_switch/simpleSwitch.h
    simple_switch/options.h
    )
    
set (PORTABLE_COMMON
    portable_common/portable.cpp
    portable_common/portable.h  
    portable_common/options.cpp
    portable_common/options.h    
    portable_common/midend.cpp
    portable_common/midend.h
    )

set (BMV2_PSA_SWITCH_SRCS
    psa_switch/main.cpp
    psa_switch/midend.cpp
    psa_switch/midend.h
    psa_switch/psaSwitch.cpp
    psa_switch/psaSwitch.h
    psa_switch/options.cpp
    psa_switch/options.h
    )

set (BMV2_PNA_NIC_SRCS
    pna_nic/main.cpp
    pna_nic/midend.cpp
    pna_nic/midend.h
    pna_nic/pnaNic.cpp
    pna_nic/pnaNic.h
    pna_nic/pnaProgramStructure.cpp
    pna_nic/pnaProgramStructure.h
    pna_nic/options.cpp
    pna_nic/options.h
    )

set (BMV2_BACKEND_COMMON_SRCS
  common/JsonObjects.cpp
  common/action.cpp
  common/controlFlowGraph.cpp
  common/deparser.cpp
  common/expression.cpp
  common/extern.cpp
  common/globals.cpp
  common/header.cpp
  common/helpers.cpp
  common/lower.cpp
  common/parser.cpp
  )

set (BMV2_BACKEND_COMMON_HDRS
  common/JsonObjects.h
  common/action.h
  common/annotations.h
  common/backend.h
  common/check_unsupported.h
  common/control.h
  common/controlFlowGraph.h
  common/deparser.h
  common/expression.h
  common/extern.h
  common/globals.h
  common/header.h
  common/helpers.h
  common/lower.h
  common/midend.h
  common/options.h
  common/parser.h
  common/sharedActionSelectorCheck.h
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bmv2.def PARENT_SCOPE)

add_library(bmv2backend STATIC ${BMV2_BACKEND_COMMON_SRCS})
target_link_libraries(bmv2backend ir-generated frontend backends-common)

add_executable(p4c-bm2-ss ${BMV2_SIMPLE_SWITCH_SRCS})
target_link_libraries (p4c-bm2-ss bmv2backend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install(TARGETS p4c-bm2-ss RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_library(portable-common STATIC ${PORTABLE_COMMON})
target_link_libraries(portable-common bmv2backend)

add_executable(p4c-bm2-psa ${BMV2_PSA_SWITCH_SRCS})
target_link_libraries (p4c-bm2-psa portable-common ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
install(TARGETS p4c-bm2-psa RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_executable(p4c-bm2-pna ${BMV2_PNA_NIC_SRCS})
target_link_libraries (p4c-bm2-pna portable-common ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
install(TARGETS p4c-bm2-pna RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

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
add_custom_target(linkbmv2
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-ss ${P4C_BINARY_DIR}/p4c-bm2-ss
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-psa ${P4C_BINARY_DIR}/p4c-bm2-psa
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CURRENT_BINARY_DIR_PATH_REL}/p4c-bm2-pna ${P4C_BINARY_DIR}/p4c-bm2-pna
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR_PATH_REL}/p4_14include ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  DEPENDS update_includes
  )
add_dependencies(p4c_driver linkbmv2)


# Tests
set(BMV2_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/run-bmv2-test.py)
set(BMV2_PTF_DRIVER "$([[ $EUID = 0 ]] || echo 'sudo -E') ${CMAKE_CURRENT_SOURCE_DIR}/run-bmv2-ptf-test.py")

set (V1_INCLUDE_PATTERNS "include.*v1model.p4")
set (P4TESTS_FOR_V1MODEL "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4")
p4c_find_tests("${P4TESTS_FOR_V1MODEL}" v1tests INCLUDE "${V1_INCLUDE_PATTERNS}")

set (BMV2_V1MODEL_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_bmv_errors/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/dash/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/fabric_*/fabric.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/omec/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pins/*.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/switch_*/switch.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
  ${v1tests}
  )

set (PSA_INCLUDE_PATTERNS "include.*psa.p4")
set (PSA_EXCLUDE_PATTERNS "include.*v1model.p4" "include.*dpdk")
set (P4TESTS_FOR_PSA
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/psa-*.p4"
  )
p4c_find_tests("${P4TESTS_FOR_PSA}" psa_tests INCLUDE "${PSA_INCLUDE_PATTERNS}" EXCLUDE "${PSA_EXCLUDE_PATTERNS}")

set (BMV2_PSA_TEST_SUITES
  ${psa_tests}
)

set (PNA_INCLUDE_PATTERNS "include.*pna.p4")
set (PNA_EXCLUDE_PATTERNS "include.*v1model.p4" "include.*dpdk")
set (P4TESTS_FOR_PNA
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/*-bmv2.p4"
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/pna-*.p4"
  )
p4c_find_tests("${P4TESTS_FOR_PNA}" pna_test_candidates INCLUDE "${PNA_INCLUDE_PATTERNS}" EXCLUDE "${PNA_EXCLUDE_PATTERNS}")
# Exclude dpdk tests from PNA test candidates
# FIXME: some of these work with BMv2 -- potentially enable later
set (pna_tests "")
foreach (t ${pna_test_candidates})
  if (NOT t MATCHES ".*-dpdk-.*.p4")
    list (APPEND pna_tests ${t})
  endif()
endforeach()

set (BMV2_PNA_TEST_SUITES
  ${pna_tests}
)

# Execute PTF tests for these files.
set (BMV2_PTF_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_16_samples/ternary2-bmv2.p4"
)

# -p to use psa switch model
# -a to pass flags to compiler
set (testExtraFlagsPSA "${testExtraFlags} -p -a='--target bmv2 --arch psa'")
set (testExtraFlagsPNA "${testExtraFlags} --use_pna -a='--target bmv2 --arch pna'")

set (XFAIL_TESTS
  # This test defines two lpm keys for a table.
  # Even though the P4 spec allows it, it doesn't really make sense in a switch
  # so we allow it to fail on BMv2.
  testdata/p4_14_samples/issue60.p4
  # compiler claims (incorrectly?) that c2 has mulitple successors, so is not supported
  testdata/p4_14_samples/issue-1426.p4
  # This test uses a feature currently unsupported in the BMv2 back-end.
  testdata/p4_16_samples/issue907-bmv2.p4
  # This test uses a table graph that is not implementable in BMv2
  testdata/p4_16_samples/issue986-bmv2.p4
  # These tests use a table key with type 'error'
  testdata/p4_16_samples/issue1062-bmv2.p4
  # This test uses an undefined extern
  testdata/p4_16_samples/issue1193-bmv2.p4
  # This test also uses a custom extern
  testdata/p4_14_samples/issue604.p4
  # This test uses an incorrect model
  testdata/p4_16_samples/issue1205-bmv2.p4
  # These psa tests are not ready to run on bmv2 yet
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  testdata/p4_16_samples/psa-example-parser-checksum.p4 # error: error.NoError: unsupported exact key expression
  testdata/p4_16_samples/psa-meter4.p4 # Compiler Bug: backends/bmv2/portable_common/portable.cpp:574: Null info
  testdata/p4_16_samples/psa-meter5.p4 # Compiler Bug: backends/bmv2/portable_common/portable.cpp:574: Null info
  testdata/p4_16_samples/psa-table-hit-miss.p4 # error: Program can not be implemented on this target since it
                                               # contains a path from table MyIC.tbl back to itself
  # This reads stack.next
  testdata/p4_16_samples/issue692-bmv2.p4
  # These test use computations in the verify/update checksum controls - unsupported
  testdata/p4_16_samples/issue1765-bmv2.p4
  testdata/p4_16_samples/issue1765-1-bmv2.p4
  # Next three use unknown externs
  testdata/p4_16_samples/issue1882-bmv2.p4
  testdata/p4_16_samples/issue1882-1-bmv2.p4
  testdata/p4_16_samples/issue2664-bmv2.p4
  # This test tries to hash (via the hash extern) a value that isn't a tuple - unsupported
  testdata/p4_16_samples/hashing-non-tuple-bmv2.p4
  # Similarly, this test hashes on a bit input.
  testdata/p4_16_samples/issue584-1-bmv2.p4
  # These pna tests are not ready to run on bmv2 yet
  testdata/p4_16_samples/pna-example-SelectByDirection.p4  # error: SelectByDirection(...): not supported
  testdata/p4_16_samples/pna-example-SelectByDirection1.p4  # error: SelectByDirection(...): not supported
  testdata/p4_16_samples/pna-example-SelectByDirection2.p4  # error: SelectByDirection(...): not supported
  testdata/p4_16_samples/pna-example-header-union.p4  # Error -11
  testdata/p4_16_samples/pna-example-header-union1.p4  # Error -11
  testdata/p4_16_samples/pna-example-pass-2.p4  # Conditional execution in actions not supported
  testdata/p4_16_samples/pna-example-tcp-connection-tracking.p4  # Conditional execution in actions not supported
  testdata/p4_16_samples/pna-extract-local-header.p4  # error: IfStatement: not supported within a deparser on this target
)

set (BMV2_PARSER_INLINE_TESTS "${P4C_SOURCE_DIR}/testdata/p4_16_samples/parser-inline/*.p4")

if (HAVE_SIMPLE_SWITCH)
  if (NOT ENABLE_SANITIZERS)
    # Does not work well with sanitizers, extern_func_module fails due to linking failures related to missing symbols.
    p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_V1MODEL_TEST_SUITES}" "${XFAIL_TESTS}")
    add_library(extern_func_module MODULE EXCLUDE_FROM_ALL "${P4C_SOURCE_DIR}/testdata/extern_modules/extern-funcs-bmv2.cpp" )
    p4c_add_test_with_args("bmv2" ${BMV2_DRIVER} FALSE "bmv2_emit_externs"
      "testdata/p4_16_samples/extern-funcs-bmv2.p4"
      "-a=--emit-externs --target-specific-switch-arg=\"--load-modules ${CMAKE_CURRENT_BINARY_DIR}/libextern_func_module.so\" --init=\"cmake --build . --target extern_func_module extern_func_module\""
      ""
    )
    p4c_add_tests("bmv2-parser-inline-opt-disabled" ${BMV2_DRIVER} "${BMV2_PARSER_INLINE_TESTS}" "")
    p4c_add_tests("bmv2-parser-inline-opt-enabled" ${BMV2_DRIVER} "${BMV2_PARSER_INLINE_TESTS}" "" "-a=--parser-inline-opt")
  endif()
else()
  MESSAGE(WARNING "BMv2 simple switch is not available, not adding v1model BMv2 tests")
endif()

if (HAVE_SIMPLE_SWITCH_GRPC)
    p4c_add_tests("bmv2-ptf" ${BMV2_PTF_DRIVER} "${BMV2_PTF_TEST_SUITES}" "")
else()
  MESSAGE(WARNING "BMv2 simple switch grpc is not available, not adding v1model BMv2 PTF tests")
endif()

if (HAVE_PSA_SWITCH)
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_PSA_TEST_SUITES}" "${XFAIL_TESTS}" ${testExtraFlagsPSA})
else()
  MESSAGE(WARNING "BMv2 PSA switch is not available, not adding PSA BMv2 tests")
endif()

if (HAVE_PNA_NIC)
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${BMV2_PNA_TEST_SUITES}" "${XFAIL_TESTS}" ${testExtraFlagsPNA})
else()
  MESSAGE(WARNING "BMv2 PNA switch is not available, not adding PNA BMv2 tests")
endif()

set (GTEST_BMV2_SOURCES
  gtest/load_ir_from_json.cpp
)
set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_BMV2_SOURCES} PARENT_SCOPE)
set (GTEST_LDADD ${GTEST_LDADD} bmv2backend PARENT_SCOPE)
