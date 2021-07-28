p4c_add_xfail_reason("dpdk"
  "Unsupported parser loop"
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected psa_implementation property value for table.* to resolve to an extern instance"
  testdata/p4_16_samples/psa-action-profile2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Not implemented"
  testdata/p4_16_samples/psa-action-selector5.p4
  testdata/p4_16_samples/psa-dpdk-table-key-consolidation-switch.p4
  testdata/p4_16_samples/psa-random.p4
  )

p4c_add_xfail_reason("dpdk"
  "LHS of meter execute statement is missing"
  testdata/p4_16_samples/psa-meter1.p4
)

p4c_add_xfail_reason("dpdk"
  "Expected atleast 2 arguments"
  testdata/p4_16_samples/psa-meter3.p4
  testdata/p4_16_samples/psa-meter7-bmv2.p4
)

# symbolic evaluator does not support verify() statement
p4c_add_xfail_reason("dpdk"
  "unknown: expected a bool"
  testdata/p4_16_samples/psa-example-parser-checksum.p4
  )

p4c_add_xfail_reason("dpdk"
  "error: AssignmentStatement"
  testdata/p4_16_samples/psa-multicast-basic-2-bmv2.p4
  testdata/p4_16_samples/psa-unicast-or-drop-bmv2.p4
  testdata/p4_16_samples/psa-example-parser-checksum.p4
)

p4c_add_xfail_reason("dpdk"
  "Not Implemented"
  testdata/p4_16_samples/psa-example-register2-bmv2.p4
  testdata/p4_16_samples/psa-register-read-write-2-bmv2.p4
  testdata/p4_16_samples/psa-end-of-ingress-test-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Error compiling"
  testdata/p4_16_samples/psa-recirculate-no-meta-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "get_hash's arg is not a ListExpression"
  testdata/p4_16_samples/psa-hash.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unknown extern function"
  testdata/p4_16_samples/psa-counter6.p4
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
  "Key field wider than 64-bit is not permitted"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err3.p4
  )
