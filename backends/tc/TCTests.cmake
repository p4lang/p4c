# SPDX-FileCopyrightText: 2025 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

set(P4TC_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-tc-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4tc_samples/*.p4")

# Every P4TC test needs the fixture, the one shared lock and the long timeout.
# One macro owns that list so a registration cannot drift from it.
macro(p4tc_set_test_properties name)
  set(__p4tc_props FIXTURES_REQUIRED P4TCFixture RESOURCE_LOCK "shared_lock" TIMEOUT 1000)
  if (${ARGC} GREATER 1)
    list(APPEND __p4tc_props WORKING_DIRECTORY ${ARGN})
  endif()
  set_tests_properties(${name} PROPERTIES ${__p4tc_props})
endmacro(p4tc_set_test_properties)

macro(p4tc_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  p4c_add_test_with_args(${tag} ${driver} ${isXfail} ${alias} ${p4test} ${test_args} "")
  p4c_test_set_name(__testname ${tag} ${alias})
  p4tc_set_test_properties(${__testname})
endmacro(p4tc_add_test_with_args)

# Register a test that runs a P4 file against an expectation file, both named
# relative to the source tree. The alias is the test's name, which is not always
# the P4 file's name because one P4 file can back several STF variants. The
# remaining arguments are driver flags the fixture needs; ARGN arrives as a list
# and the driver takes one flag string, so they are joined. They are counted with
# ARGC and not tested with if (ARGN): inside a macro the argument names are
# substituted as text, so if (ARGN) does not see the list.
macro(p4tc_add_stf_test alias p4test stffile)
  set(__p4tc_args "-tf ${P4C_SOURCE_DIR}/${stffile}")
  if (${ARGC} GREATER 3)
    string(REPLACE ";" " " __p4tc_flags "${ARGN}")
    set(__p4tc_args "${__p4tc_flags} ${__p4tc_args}")
  endif()
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "${alias}" "${p4test}"
    "${__p4tc_args}" "")
endmacro(p4tc_add_stf_test)

include(CheckLinuxKernel)
check_minimum_kernel_version("5.0.0" SUPPORTS_KERNEL)
check_minimum_linux_libc_version("5.0.0" SUPPORTS_LIBC)

if (NOT SUPPORTS_KERNEL OR NOT SUPPORTS_LIBC)
  message(STATUS "Disabling P4TC tests.")
  return()
endif()

# A specific version of clang is required for the P4TC tests.
set (P4TC_CLANG "clang-19")
find_program(P4TC_CLANG_EXEC ${P4TC_CLANG})

if (NOT P4TC_CLANG_EXEC)
  message(STATUS "Disabling P4TC tests.")
  return()
endif()

set(ENABLE_P4TC_STF_TESTS ON)
# brctl is required for the P4TC STF tests.
find_program(BRCTL_EXEC brctl)
if(NOT BRCTL_EXEC)
  message(STATUS "brctl not found. Disabling P4TC STF tests.")
  set(ENABLE_P4TC_STF_TESTS OFF)
endif()

# Only enable P4TC and P4TC STF tests when all required tools are available.
p4c_add_tests("p4tc" ${P4TC_COMPILER_DRIVER} "${P4_16_SUITES}" "")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
foreach(mode counter emission parser deparser deparser-bits reject-error narrow-cache vlan
             comparison advance bitfields arithmetic)
  add_test(NAME "p4tc-codegen/${mode}"
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/run.py
      --compiler $<TARGET_FILE:p4c-pna-p4tc> --mode ${mode})
  set_tests_properties("p4tc-codegen/${mode}" PROPERTIES
    SKIP_RETURN_CODE 77 TIMEOUT 180 LABELS "p4tc-codegen")
endforeach()

add_test(NAME "p4tc-codegen/register-init"
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/register_init.py
    --compiler $<TARGET_FILE:p4c-pna-p4tc>)
set_tests_properties("p4tc-codegen/register-init" PROPERTIES
  SKIP_RETURN_CODE 77 TIMEOUT 180 LABELS "p4tc-codegen")

# The grammar test reads the emitted constants with iproute2's own parser, so it
# needs a built iproute2 tree. The P4TC CI job builds one for the stf tests, so
# take that tree when it is there; otherwise the test skips, and P4TC_IPROUTE2
# names a tree from the environment instead. The negative half of the test needs
# the bit<128>/IPv6-mask grammar fix in that tree: a parser that predates it is
# reported and only fails when the test is asked to expect the fix.
set(P4TC_IPROUTE2_DIR "${P4C_SOURCE_DIR}/backends/tc/runtime/iproute2-p4tc-pub"
  CACHE PATH "A built iproute2 tree, used by p4tc-codegen/grammar")
option(P4TC_GRAMMAR_EXPECT_FIXED
  "Require the iproute2 parser to refuse malformed bit<128>/IPv6-mask text" OFF)
set(__p4tc_grammar_args --compiler $<TARGET_FILE:p4c-pna-p4tc>)
if (P4TC_IPROUTE2_DIR)
  list(APPEND __p4tc_grammar_args --iproute2 "${P4TC_IPROUTE2_DIR}")
endif()
if (P4TC_GRAMMAR_EXPECT_FIXED)
  list(APPEND __p4tc_grammar_args --expect-fixed)
endif()
add_test(NAME "p4tc-codegen/grammar"
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/codegen/grammar.py
    ${__p4tc_grammar_args})
set_tests_properties("p4tc-codegen/grammar" PROPERTIES
  SKIP_RETURN_CODE 77 TIMEOUT 300 LABELS "p4tc-codegen")

p4c_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE
  "testdata/p4_16_samples/tc-register-init.p4"
  "testdata/p4_16_samples/tc-register-init.p4" "" "")

p4c_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE
  "testdata/p4_16_samples/tc-large-header-init.p4"
  "testdata/p4_16_samples/tc-large-header-init.p4" "" "")
p4c_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE
  "testdata/p4_16_samples/tc-parser-advance.p4"
  "testdata/p4_16_samples/tc-parser-advance.p4" "" "")

if (ENABLE_P4TC_STF_TESTS)
  # Setup fixture
  add_test(NAME p4tc_setup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/setup "https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest")
  set_tests_properties(p4tc_setup PROPERTIES FIXTURES_SETUP P4TCFixture)

  add_test(NAME p4tc_cleanup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/cleanup)
  set_tests_properties(p4tc_cleanup PROPERTIES FIXTURES_CLEANUP P4TCFixture)

  p4tc_add_stf_test(testdata/p4tc_samples_stf/arp_respond.p4
    testdata/p4tc_samples_stf/arp_respond.p4
    testdata/p4tc_samples_stf/arp_respond.stf)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/simple_l3.p4
    testdata/p4tc_samples_stf/simple_l3.p4
    testdata/p4tc_samples_stf/simple_l3.stf)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/calc_128.p4
    testdata/p4tc_samples_stf/calc_128.p4
    testdata/p4tc_samples_stf/calc_128.stf)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/redirect_l2.p4
    testdata/p4tc_samples_stf/redirect_l2.p4
    testdata/p4tc_samples_stf/redirect_l2.stf)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/routing_default
    testdata/p4tc_samples_stf/routing.p4
    testdata/p4tc_samples_stf/routing_default.stf
    -e InternetChecksum)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/routing.p4
    testdata/p4tc_samples_stf/routing.p4
    testdata/p4tc_samples_stf/routing.stf
    -e InternetChecksum)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/ipip_encap
    testdata/p4tc_samples_stf/ipip.p4
    testdata/p4tc_samples_stf/ipip_encap.stf
    -e InternetChecksum)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/ipip_decap
    testdata/p4tc_samples_stf/ipip.p4
    testdata/p4tc_samples_stf/ipip_decap.stf
    -e InternetChecksum)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/varbit_pop
    testdata/p4tc_samples_stf/varbit_pop.p4
    testdata/p4tc_samples_stf/varbit_pop.stf
    -e InternetChecksum)
  p4tc_add_stf_test(testdata/p4tc_samples_stf/varbit_reflect
    testdata/p4tc_samples_stf/varbit_reflect.p4
    testdata/p4tc_samples_stf/varbit_reflect.stf)
  foreach(comparison_test mac_comparisons array_comparisons signed24_comparisons
                           signed_scalar_comparisons const_range)
    p4tc_add_stf_test(testdata/p4tc_samples_stf/${comparison_test}
      testdata/p4tc_samples_stf/${comparison_test}.p4
      testdata/p4tc_samples_stf/${comparison_test}.stf)
  endforeach()
  add_test(NAME "p4tc/testdata/p4_16_samples/tc-large-header-init-stf"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-large-header-init.p4
      -tf ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-large-header-init.stf)
  p4tc_set_test_properties("p4tc/testdata/p4_16_samples/tc-large-header-init-stf" ${P4C_BINARY_DIR})
  add_test(NAME "p4tc/testdata/p4_16_samples/tc-parser-advance-stf"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-parser-advance.p4
      -tf ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-parser-advance.stf)
  p4tc_set_test_properties("p4tc/testdata/p4_16_samples/tc-parser-advance-stf" ${P4C_BINARY_DIR})
  add_test(NAME "p4tc/testdata/p4_16_samples/tc-codegen-counter-stf"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-counter.p4
      -tf ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-counter.stf)
  p4tc_set_test_properties("p4tc/testdata/p4_16_samples/tc-codegen-counter-stf" ${P4C_BINARY_DIR})
  add_test(NAME "p4tc/testdata/p4_16_samples/tc-codegen-emission-stf"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-emission.p4
      -tf ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-emission.stf)
  p4tc_set_test_properties("p4tc/testdata/p4_16_samples/tc-codegen-emission-stf" ${P4C_BINARY_DIR})
  add_test(NAME "p4tc/testdata/p4_16_samples/tc-codegen-arithmetic-stf"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-arithmetic.p4
      -tf ${P4C_SOURCE_DIR}/testdata/p4_16_samples/tc-codegen-arithmetic.stf)
  p4tc_set_test_properties("p4tc/testdata/p4_16_samples/tc-codegen-arithmetic-stf" ${P4C_BINARY_DIR})

  # Some fixtures have constants in their template and no stf file, so nothing
  # ever hands that template to tc. Load it in the guest and stop: the check is
  # that tc accepts every constant the compiler wrote.
  foreach(load_test p4_16_samples/tc-register-init p4tc_samples/const_typed_keys
                     p4tc_samples/test_ipv6_example)
    add_test(NAME "p4tc-load/${load_test}"
      COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
        ${P4C_SOURCE_DIR}/testdata/${load_test}.p4 -P)
    p4tc_set_test_properties("p4tc-load/${load_test}" ${P4C_BINARY_DIR})
  endforeach()
endif()
