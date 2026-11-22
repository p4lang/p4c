# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

# Inputs shared with p4test
set(BMV2_DRIVER ${P4C_SOURCE_DIR}/backends/bmv2/run-bmv2-test.py)

set(BMV2_V1MODEL_INPUTS
  "testdata/bmv2/v1model/*.p4"
  "testdata/bmv2/v1model/dash/*.p4"
  "testdata/bmv2/v1model/fabric_*/fabric.p4"
  "testdata/bmv2/v1model/omec/*.p4"
  "testdata/bmv2/v1model/pins/*.p4")
set(P4TEST_SUITES ${P4TEST_SUITES}
  ${BMV2_V1MODEL_INPUTS}
  "testdata/bmv2/psa/*.p4"
  "testdata/bmv2/pna/*.p4"
  PARENT_SCOPE)

include(Bmv2Xfail.cmake)

# v1model: simple_switch
set(BMV2_V1MODEL_TEST_SUITES ${BMV2_V1MODEL_INPUTS} "testdata/bmv2/v1model/errors/*.p4")
if(SUPPORT_P4_14)
  list(APPEND BMV2_V1MODEL_TEST_SUITES "testdata/p4_14/*.p4" "testdata/p4_14/switch_*/switch.p4")
endif()

if(NOT HAVE_SIMPLE_SWITCH)
  message(WARNING "Skipping v1model tests: simple_switch is missing.")
elseif(NOT ENABLE_SANITIZERS)
  # The extern module fails to link in sanitizer builds.
  # The extern program needs the module and compiler flags registered below.
  set(extern_input "testdata/bmv2/v1model/extern-funcs-bmv2.p4")
  p4c_find_test_names("${BMV2_V1MODEL_TEST_SUITES}" v1model_tests)
  list(REMOVE_ITEM v1model_tests "${extern_input}")
  p4c_add_test_list("bmv2" ${BMV2_DRIVER} "${v1model_tests}" "${XFAIL_TESTS}")

  add_library(extern_func_module MODULE EXCLUDE_FROM_ALL
    v1model/extern-funcs-bmv2.cpp)
  string(CONCAT extern_args "-a=--emit-externs "
    "--target-specific-switch-arg=\"--load-modules ${CMAKE_CURRENT_BINARY_DIR}/libextern_func_module.so\" "
    "--init=\"cmake --build . --target extern_func_module extern_func_module\"")
  p4c_add_test_with_args("bmv2" ${BMV2_DRIVER} FALSE "bmv2_emit_externs"
    "${extern_input}" "${extern_args}" "")

  set(parser_inputs "testdata/bmv2/v1model/parser-inline/*.p4")
  p4c_add_tests("bmv2-parser-inline-opt-disabled" ${BMV2_DRIVER} "${parser_inputs}" "")
  p4c_add_tests("bmv2-parser-inline-opt-enabled" ${BMV2_DRIVER} "${parser_inputs}" ""
    "-a=--parser-inline-opt")
endif()

if(HAVE_SIMPLE_SWITCH_GRPC)
  set(ptf_runner "${P4C_SOURCE_DIR}/backends/bmv2/run-bmv2-ptf-test.py")
  set(ptf_driver "$([[ $EUID = 0 ]] || echo 'sudo -E') ${ptf_runner} --use-veth")
  p4c_add_tests("bmv2-ptf" ${ptf_driver} "testdata/bmv2/v1model/ternary2-bmv2.p4" "")
else()
  message(WARNING "Skipping v1model PTF tests: simple_switch_grpc is missing.")
endif()

# PSA and PNA each use their own switch and also consume shared architecture tests.
function(add_portable_suite arch)
  set(inputs
    "testdata/bmv2/${arch}/*.p4"
    "testdata/shared/${arch}/*.p4"
    "testdata/shared/${arch}/errors/*.p4")
  set(flags "${testExtraFlags} --target bmv2 --arch ${arch}")
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "${inputs}" "${XFAIL_TESTS}" "${flags}")
  p4c_add_tests("bmv2" ${BMV2_DRIVER} "testdata/bmv2/${arch}/errors/*.p4" "" "${flags}")
endfunction()

if(HAVE_PSA_SWITCH)
  add_portable_suite(psa)
else()
  message(WARNING "Skipping BMv2 PSA tests: psa_switch is missing.")
endif()

if(HAVE_PNA_NIC)
  add_portable_suite(pna)
else()
  message(WARNING "Skipping BMv2 PNA tests: pna_nic is missing.")
endif()
