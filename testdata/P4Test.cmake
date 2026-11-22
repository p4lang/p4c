# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

# Tests

set(P4TEST_DRIVER ${P4C_SOURCE_DIR}/backends/p4test/run-p4-sample.py)

# Builds a list of tests for which P4Info generation is not supported. It makes
# sense not to fail the test just because P4Info generation fails. It probably
# would not make sense to run all the tests twice (one with and one without
# P4Runtime).
set (P4RUNTIME_EXCLUDE
  # error not supported in static entries, not strictly required for P4Info
  # generation though
  testdata/bmv2/v1model/issue1062-bmv2.p4
  # P4Info generation fails (as expected with the current P4Runtime spec) with:
  # Unsupported type parameter for Value Set; this version of P4Runtime requires
  # that when the type parameter of a Value Set is a struct, all the fields of
  # the struct must be of type bit<W>, but inner is not
  testdata/core/pvs-nested-struct.p4
  # The tests below use type `error` for table key which P4Runtime 1.x
  # does not support.
  # table t_exact key named m.my_err is type error
  testdata/bmv2/v1model/issue1062-1-bmv2.p4
  # table t key named meta.err is type error
  testdata/core/issue1304.p4
  # table parser_error_count_and_convert key named istd.parser_error
  # is type error
  testdata/shared/psa/psa-example-parser-checksum.p4
)

if(ENABLE_BMV2 OR ENABLE_DPDK)
  list(APPEND P4TEST_SUITES "testdata/shared/psa/*.p4" "testdata/shared/pna/*.p4")
endif()
p4c_add_tests_w_p4runtime("p4" ${P4TEST_DRIVER} "${P4TEST_SUITES}" "" "${P4RUNTIME_EXCLUDE}" "-a '--maxErrorCount 100'")

if(ENABLE_BMV2)
  set (P4TEST_PARSER_INLINE_TESTS "${P4C_SOURCE_DIR}/testdata/bmv2/v1model/parser-inline/*.p4")
  p4c_add_tests("p4" ${P4TEST_DRIVER} "${P4TEST_PARSER_INLINE_TESTS}" "" "-a '--maxErrorCount 100 --parser-inline-opt'")
endif()

set (P4TEST_ERRORS
  "${P4C_SOURCE_DIR}/testdata/core/errors/*.p4")
if (SUPPORT_P4_14)
  list(APPEND P4TEST_ERRORS "${P4C_SOURCE_DIR}/testdata/p4_14/errors/*.p4")
endif()
# Some programs pass the frontend and fail only when generating P4Info (issue 1803).
p4c_add_tests_w_p4runtime("err" ${P4TEST_DRIVER} "${P4TEST_ERRORS}" "" "" "-a '--maxErrorCount 100'")

if (SUPPORT_P4_14)
  set(P4_14_SUITES
    "testdata/p4_14/*.p4"
    "testdata/p4_14/switch_*/switch.p4")
  p4c_add_tests("p14_to_16" ${P4TEST_DRIVER} "${P4_14_SUITES}" "")
  set_tests_properties("p14_to_16/testdata/p4_14/switch_20160512/switch.p4" PROPERTIES TIMEOUT 1000)
endif()
