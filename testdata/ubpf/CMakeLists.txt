# SPDX-FileCopyrightText: 2019 Orange
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

set(P4TEST_SUITES ${P4TEST_SUITES} "testdata/ubpf/*.p4" PARENT_SCOPE)

set(runner "${P4C_SOURCE_DIR}/backends/ubpf/run-ubpf-test.py")
set(UBPF_DRIVER "${runner} -t ubpf -c \"${P4C_BINARY_DIR}/p4c-ubpf\"")

# The extern programs need a C implementation in addition to the P4 input.
file(GLOB UBPF_TEST_SUITES "${CMAKE_CURRENT_SOURCE_DIR}/*.p4")
list(REMOVE_ITEM UBPF_TEST_SUITES
  "${CMAKE_CURRENT_SOURCE_DIR}/ubpf_hash_extern.p4"
  "${CMAKE_CURRENT_SOURCE_DIR}/ubpf_checksum_extern.p4")
p4c_add_tests("ubpf" ${UBPF_DRIVER} "${UBPF_TEST_SUITES}" "")

foreach(extern hash checksum)
  set(input "testdata/ubpf/ubpf_${extern}_extern.p4")
  set(source "${CMAKE_CURRENT_SOURCE_DIR}/extern-${extern}-ubpf.c")
  p4c_add_test_with_args("ubpf" ${UBPF_DRIVER} FALSE "${input}" "${input}"
    "--extern-file ${source}" "")
endforeach()
