# SPDX-FileCopyrightText: 2020 Intel Corp.
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

# Compiler tests
set(DPDK_COMPILER_DRIVER "${P4C_SOURCE_DIR}/backends/dpdk/run-dpdk-test.py")

set(dpdk_inputs
  "testdata/dpdk/psa/*.p4"
  "testdata/dpdk/pna/*.p4"
  "testdata/dpdk/pna/dash/*.p4")
set(P4TEST_SUITES ${P4TEST_SUITES} ${dpdk_inputs} PARENT_SCOPE)

set(dpdk_suites ${dpdk_inputs})
foreach(arch psa pna)
  list(APPEND dpdk_suites
    "testdata/shared/${arch}/*.p4"
    "testdata/shared/${arch}/errors/*.p4"
    "testdata/dpdk/${arch}/errors/*.p4")
endforeach()
p4c_add_tests("dpdk" ${DPDK_COMPILER_DRIVER} "${dpdk_suites}" "" "--bfrt")
include(DpdkXfail.cmake)

# Packet tests need infrap4d and a DPDK target installation.
find_program(INFRAP4D infrap4d PATHS ${IPDK_INSTALL_DIR}/sbin)
if(NOT INFRAP4D OR NOT EXISTS "${IPDK_INSTALL_DIR}")
  message(WARNING "Skipping DPDK PTF tests: infrap4d or the DPDK target installation is missing.")
  return()
endif()

set(DPDK_PTF_DRIVER "${P4C_SOURCE_DIR}/backends/dpdk/run-dpdk-ptf-test.py")
set(DPDK_PTF_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/dpdk/pna/pna-dpdk-add_on_miss0.p4"
  "${P4C_SOURCE_DIR}/testdata/dpdk/pna/pna-dpdk-mux-concat.p4"
  "${P4C_SOURCE_DIR}/testdata/dpdk/pna/pna-dpdk-local-key.p4"
  "${P4C_SOURCE_DIR}/testdata/dpdk/pna/pna-dpdk-ptf-ports.p4"
  "${P4C_SOURCE_DIR}/testdata/dpdk/pna/pna-dpdk-small_sample.p4"
)

p4c_add_tests("dpdk-ptf" ${DPDK_PTF_DRIVER} "${DPDK_PTF_TEST_SUITES}" ""
  "--ipdk-install-dir=${IPDK_INSTALL_DIR} --compiler=${P4C_BINARY_DIR}/p4c-dpdk -ll=DEBUG ")

foreach(test IN LISTS DPDK_PTF_TEST_SUITES)
  file(RELATIVE_PATH test_name ${P4C_SOURCE_DIR} ${test})
  set_tests_properties(dpdk-ptf/${test_name} PROPERTIES RESOURCE_LOCK dpdk)
endforeach()
