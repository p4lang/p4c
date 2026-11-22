# SPDX-FileCopyrightText: 2025 The P4 Language Consortium
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

set(P4TEST_SUITES ${P4TEST_SUITES} "testdata/tc/*.p4" PARENT_SCOPE)

# The TC runner needs Linux 5.0 or newer and clang-19.
include(CheckLinuxKernel)
check_minimum_kernel_version("5.0.0" SUPPORTS_KERNEL)
check_minimum_linux_libc_version("5.0.0" SUPPORTS_LIBC)
find_program(P4TC_CLANG_EXEC clang-19)
if(NOT SUPPORTS_KERNEL OR NOT SUPPORTS_LIBC OR NOT P4TC_CLANG_EXEC)
  message(STATUS "Skipping P4TC tests: Linux 5.0 headers, kernel, and clang-19 are required.")
  return()
endif()

# Compiler tests
set(tc_backend "${P4C_SOURCE_DIR}/backends/tc")
set(P4TC_COMPILER_DRIVER "${tc_backend}/run-tc-test.py")
set(compiler_inputs
  "testdata/tc/compiler/*.p4"
  "testdata/tc/compiler/errors/*.p4"
  "testdata/tc/tc-register-init.p4"
  "testdata/tc/tc-large-header-init.p4"
  "testdata/tc/tc-parser-advance.p4")
p4c_add_tests("p4tc" ${P4TC_COMPILER_DRIVER} "${compiler_inputs}" "")

# Generated code checks
find_package(Python3 REQUIRED COMPONENTS Interpreter)
function(add_codegen_test name script)
  add_test(NAME "p4tc-codegen/${name}"
    COMMAND ${Python3_EXECUTABLE} "${tc_backend}/tests/codegen/${script}"
      --compiler $<TARGET_FILE:p4c-pna-p4tc> ${ARGN})
  set_tests_properties("p4tc-codegen/${name}" PROPERTIES
    SKIP_RETURN_CODE 77 TIMEOUT 180 LABELS "p4tc-codegen")
endfunction()

foreach(mode counter emission parser deparser deparser-bits reject-error narrow-cache vlan
             comparison advance bitfields arithmetic)
  add_codegen_test(${mode} run.py --mode ${mode})
endforeach()
add_codegen_test(register-init register_init.py)

# Packet tests share a runtime fixture and must run one at a time.
find_program(BRCTL_EXEC brctl)
if(NOT BRCTL_EXEC)
  message(STATUS "Skipping P4TC STF tests: brctl is missing.")
  return()
endif()

add_test(NAME p4tc_setup COMMAND bash "${tc_backend}/runtime/setup"
  "https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest")
set_tests_properties(p4tc_setup PROPERTIES FIXTURES_SETUP P4TCFixture)
add_test(NAME p4tc_cleanup COMMAND bash "${tc_backend}/runtime/cleanup")
set_tests_properties(p4tc_cleanup PROPERTIES FIXTURES_CLEANUP P4TCFixture)

set(packet_properties FIXTURES_REQUIRED P4TCFixture RESOURCE_LOCK shared_lock TIMEOUT 1000)

# The alias names the CTest case; the P4 and STF inputs can have different names.
function(add_stf_test alias program packets)
  set(test_name "testdata/tc/stf/${alias}")
  set(input "testdata/tc/stf/${program}.p4")
  set(args "-tf ${CMAKE_CURRENT_SOURCE_DIR}/stf/${packets}.stf")
  if(ARGN)
    set(args "-e ${ARGN} ${args}")
  endif()
  p4c_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "${test_name}" "${input}" "${args}" "")
  set_tests_properties("p4tc/${test_name}" PROPERTIES ${packet_properties})
endfunction()

foreach(program arp_respond simple_l3 calc_128 redirect_l2)
  add_stf_test(${program}.p4 ${program} ${program})
endforeach()

add_stf_test(routing_default routing routing_default InternetChecksum)
add_stf_test(routing.p4      routing routing         InternetChecksum)
add_stf_test(ipip_encap      ipip    ipip_encap      InternetChecksum)
add_stf_test(ipip_decap      ipip    ipip_decap      InternetChecksum)
add_stf_test(varbit_pop      varbit_pop     varbit_pop InternetChecksum)
add_stf_test(varbit_reflect  varbit_reflect varbit_reflect)

foreach(program mac_comparisons array_comparisons signed24_comparisons
                signed_scalar_comparisons const_range)
  add_stf_test(${program} ${program} ${program})
endforeach()

# These compiler regressions also have packet tests beside their P4 sources.
foreach(program tc-large-header-init tc-parser-advance tc-codegen-counter
                tc-codegen-emission tc-codegen-arithmetic)
  set(name "p4tc/testdata/tc/${program}-stf")
  add_test(NAME "${name}"
    COMMAND ${P4TC_COMPILER_DRIVER} ${P4C_SOURCE_DIR}
      "${CMAKE_CURRENT_SOURCE_DIR}/${program}.p4" -tf "${CMAKE_CURRENT_SOURCE_DIR}/${program}.stf")
  set_tests_properties("${name}" PROPERTIES ${packet_properties} WORKING_DIRECTORY ${P4C_BINARY_DIR})
endforeach()
