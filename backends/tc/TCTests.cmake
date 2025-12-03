
set(P4TC_COMPILER_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-tc-test.py")

set (P4_16_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4tc_samples/*.p4")

macro(p4tc_add_test_with_args tag driver isXfail alias p4test test_args cmake_args)
  p4c_add_test_with_args(${tag} ${driver} ${isXfail} ${alias} ${p4test} ${test_args} "")
  p4c_test_set_name(__testname ${tag} ${alias})
  set_tests_properties(${__testname} PROPERTIES FIXTURES_REQUIRED P4TCFixture RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES RESOURCE_LOCK "shared_lock")
  set_tests_properties(${__testname} PROPERTIES TIMEOUT 1000)
endmacro(p4tc_add_test_with_args)

include(CheckLinuxKernel)
check_minimum_kernel_version("5.0.0" SUPPORTS_KERNEL)
check_minimum_linux_libc_version("5.0.0" SUPPORTS_LIBC)

if (NOT SUPPORTS_KERNEL OR NOT SUPPORTS_LIBC)
  message(STATUS "Disabling P4TC tests.")
  return()
endif()

# A specific version of clang is required for the P4TC tests.
set (P4TC_CLANG "clang-15")
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

if (ENABLE_P4TC_STF_TESTS)
  # Setup fixture
  add_test(NAME p4tc_setup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/setup "https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest")
  set_tests_properties(p4tc_setup PROPERTIES FIXTURES_SETUP P4TCFixture)

  add_test(NAME p4tc_cleanup COMMAND bash ${P4C_SOURCE_DIR}/backends/tc/runtime/cleanup)
  set_tests_properties(p4tc_cleanup PROPERTIES FIXTURES_CLEANUP P4TCFixture)

  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/arp_respond.p4" "testdata/p4tc_samples_stf/arp_respond.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/arp_respond.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/simple_l3.p4" "testdata/p4tc_samples_stf/simple_l3.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/simple_l3.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/calc_128.p4" "testdata/p4tc_samples_stf/calc_128.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/calc_128.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/redirect_l2.p4" "testdata/p4tc_samples_stf/redirect_l2.p4" "-tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/redirect_l2.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/routing_default" "testdata/p4tc_samples_stf/routing.p4" "-e InternetChecksum -tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/routing_default.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/routing.p4" "testdata/p4tc_samples_stf/routing.p4" "-e InternetChecksum -tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/routing.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/ipip_encap" "testdata/p4tc_samples_stf/ipip.p4" "-e InternetChecksum -tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/ipip_encap.stf" "")
  p4tc_add_test_with_args("p4tc" ${P4TC_COMPILER_DRIVER} FALSE "testdata/p4tc_samples_stf/ipip_decap" "testdata/p4tc_samples_stf/ipip.p4" "-e InternetChecksum -tf ${P4C_SOURCE_DIR}/testdata/p4tc_samples_stf/ipip_decap.stf" "")
endif()
