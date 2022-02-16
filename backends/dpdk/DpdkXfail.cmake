p4c_add_xfail_reason("dpdk"
  "error: Error when generating BF-RT info for 'Digest' .*: packed type is too complex"
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected packet length argument for"
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "error: Action parameter color has a type which is not bit<>, int<>, bool, type or serializable enum"
  testdata/p4_16_samples/psa-meter1.p4
  )

p4c_add_xfail_reason("dpdk"
  "error: Name .* is used for multiple direct counter objects in the P4Info message"
  testdata/p4_16_samples/psa-counter6.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected psa_implementation property value for table.* to resolve to an extern instance"
  testdata/p4_16_samples/psa-action-profile2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Not implemented"
  testdata/p4_16_samples/psa-random.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected atleast 2 arguments"
  testdata/p4_16_samples/psa-meter3.p4
  testdata/p4_16_samples/psa-meter7-bmv2.p4
)

p4c_add_xfail_reason("dpdk"
  "Error compiling"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err3.p4
  )

p4c_add_xfail_reason("dpdk"
  "get_hash's arg is not a ListExpression"
  testdata/p4_16_samples/psa-hash.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unknown extern function"
  testdata/p4_16_samples/psa-example-parser-checksum.p4
  testdata/p4_16_samples/psa-meter6.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unhandled declaration type"
  testdata/p4_16_samples/psa-test.p4
  )

p4c_add_xfail_reason("dpdk"
  "Only one LPM match field is permitted per table"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err1.p4
  )

p4c_add_xfail_reason("dpdk"
  "Non 'exact' match kind not permitted"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Non Type_Bits type bool for expression"
  testdata/p4_16_samples/pna-dpdk-parser-state-err.p4
  )
