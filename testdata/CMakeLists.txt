# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

# Backend suites append the programs that p4test can also run.
set(P4TEST_SUITES "testdata/core/*.p4")

if(ENABLE_BMV2)
  add_subdirectory(bmv2)
endif()
if(ENABLE_DPDK)
  add_subdirectory(dpdk)
endif()
if(ENABLE_EBPF)
  add_subdirectory(ebpf)
endif()
if(ENABLE_P4TC)
  add_subdirectory(tc)
endif()
if(ENABLE_UBPF)
  add_subdirectory(ubpf)
endif()

if(ENABLE_P4TEST)
  include(P4Test.cmake)
endif()
if(ENABLE_P4C_GRAPHS)
  add_subdirectory(graphs)
endif()
