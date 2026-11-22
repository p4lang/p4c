# SPDX-FileCopyrightText: 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

p4c_add_xfail_reason("dpdk"
  "use dpdk specific `dpdk_execute` method"
  testdata/dpdk/psa/psa-example-dpdk-meter-execute-err.p4
  testdata/shared/psa/psa-meter3.p4
  testdata/shared/psa/psa-meter7-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Meter function execute not implemented, use dpdk_execute"
  testdata/shared/psa/psa-meter1.p4
  )

p4c_add_xfail_reason("dpdk"
  "Not implemented"
  testdata/shared/psa/psa-random.p4
  )

p4c_add_xfail_reason("dpdk"
"not implemented"
testdata/shared/psa/psa-example-dpdk-byte-alignment_4.p4
  )

p4c_add_xfail_reason("dpdk"
  "Error compiling"
  testdata/dpdk/pna/pna-dpdk-wrong-warning.p4
  testdata/dpdk/pna/pna-dpdk-invalid-hdr-warnings5.p4
  testdata/dpdk/pna/pna-dpdk-invalid-hdr-warnings6.p4
  testdata/dpdk/pna/pna-dpdk-header-union-stack2.p4
  )

p4c_add_xfail_reason("dpdk"
  "declaration not found"
  testdata/dpdk/psa/psa-dpdk-header-union-typedef.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unknown extern function"
  testdata/shared/psa/psa-example-digest-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unhandled declaration type"
  testdata/shared/psa/psa-test.p4
  )

p4c_add_xfail_reason("dpdk"
  "Direct counters and direct meters are unsupported for wildcard match"
  testdata/shared/psa/psa-example-counters-bmv2.p4
  )
