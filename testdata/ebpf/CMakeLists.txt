# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

set(P4TEST_SUITES ${P4TEST_SUITES}
  "testdata/ebpf/classic/*.p4"
  "testdata/ebpf/xdp/*.p4"
  PARENT_SCOPE)

# Runners
set(runner "${P4C_SOURCE_DIR}/backends/ebpf/run-ebpf-test.py")
set(compiler_arg "-c \"${P4C_BINARY_DIR}/p4c-ebpf\"")
set(EBPF_DRIVER_KERNEL "${runner} -t kernel ${compiler_arg}")
set(EBPF_DRIVER_BCC "${runner} -t bcc ${compiler_arg}")
set(EBPF_DRIVER_TEST "${runner} -t test ${compiler_arg}")

set (XFAIL_TESTS_BCC
  # ternary not implemented for BCC
  ${P4C_SOURCE_DIR}/testdata/ebpf/classic/ternary_ebpf.p4
  )
set (XFAIL_TESTS_TEST
  # lpm and ternary not implemented for stf tests
  ${P4C_SOURCE_DIR}/testdata/ebpf/classic/lpm_ebpf.p4
  ${P4C_SOURCE_DIR}/testdata/ebpf/classic/ternary_ebpf.p4
  )

# The three runtime drivers share the classic programs. Testgen also uses op_bin
# and calc-ebpf, and the two extern tests below need extra compiler arguments.
file(GLOB EBPF_TEST_SUITES "${CMAKE_CURRENT_SOURCE_DIR}/classic/*.p4")
foreach(input ebpf_headers op_bin calc-ebpf ebpf_checksum_extern ebpf_conntrack_extern)
  list(REMOVE_ITEM EBPF_TEST_SUITES "${CMAKE_CURRENT_SOURCE_DIR}/classic/${input}.p4")
endforeach()
# Kernel tests for XDP won't work on other targets (e.g. missing bpf_xdp_* helpers).
set (EBPF_KERNEL_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/ebpf/xdp/*.p4"
  )
set (EBPF_ERRORS_SUITES
  "${P4C_SOURCE_DIR}/testdata/ebpf/classic/errors/*.p4"
  )

include(CheckLinuxKernel)
check_minimum_kernel_version("4.15.0" SUPPORTS_KERNEL)
check_minimum_linux_libc_version("4.15.0" SUPPORTS_LIBC)
if (NOT SUPPORTS_LIBC)
  message(WARNING "Disabling eBPF kernel tests.")
  set (SUPPORTS_KERNEL FALSE)
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
    set (SUPPORTS_KERNEL FALSE)
  endif()
else()
  message(WARNING
    "Did not find an LLVM version that can compile the eBPF kernel tests...\n"
    "'llvm-config' reason: ${LLVM_PACKAGE_VERSION}\n"
    "'llvm-config' return value: ${LLVM_RET}" )
  set (SUPPORTS_KERNEL FALSE)
endif()

# check for the libbpf library
find_library(LIBBPF NAMES bpf HINTS "${P4C_SOURCE_DIR}/backends/ebpf/runtime/install/libbpf/")
if (LIBBPF)
  message(STATUS "Found libbpf library at ${LIBBPF}")
else()
  message(WARNING "Missing the libbpf dependency, disabling kernel tests."
  " You can install libbpf by running './build_libbpf' in the "
  "${P4C_SOURCE_DIR}/backends/ebpf folder.")
  set (SUPPORTS_KERNEL FALSE)
endif()

# Extern tests also compile a C implementation of the extern.
function(add_ebpf_extern suite driver name xfail)
  set(input "testdata/ebpf/classic/ebpf_${name}_extern.p4")
  set(source "${CMAKE_CURRENT_SOURCE_DIR}/classic/extern-${name}-ebpf.c")
  p4c_add_test_with_args("${suite}" "${driver}" ${xfail} "${input}" "${input}"
    "--extern-file ${source}" "")
endfunction()

# Kernel runner
if (SUPPORTS_KERNEL)
  p4c_add_tests("ebpf-kernel" ${EBPF_DRIVER_KERNEL} "${EBPF_TEST_SUITES}" "")
  p4c_add_tests("ebpf-kernel" ${EBPF_DRIVER_KERNEL} "${EBPF_KERNEL_TEST_SUITES}" "")
  add_ebpf_extern("ebpf-kernel" "${EBPF_DRIVER_KERNEL}" conntrack FALSE)
  add_ebpf_extern("ebpf-kernel" "${EBPF_DRIVER_KERNEL}" checksum FALSE)
endif()

# BCC and userspace runners
p4c_add_tests("ebpf-bcc" ${EBPF_DRIVER_BCC} "${EBPF_TEST_SUITES}" "${XFAIL_TESTS_BCC}")
p4c_add_tests("ebpf" ${EBPF_DRIVER_TEST} "${EBPF_TEST_SUITES}" "${XFAIL_TESTS_TEST}")
p4c_add_tests("ebpf-errors" ${EBPF_DRIVER_TEST} ${EBPF_ERRORS_SUITES} "${XFAIL_TESTS_TEST}")

add_ebpf_extern("ebpf" "${EBPF_DRIVER_TEST}" checksum FALSE)
# The userspace runner cannot add conntrack tables dynamically.
add_ebpf_extern("ebpf" "${EBPF_DRIVER_TEST}" conntrack TRUE)
