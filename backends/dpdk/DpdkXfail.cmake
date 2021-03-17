p4c_add_xfail_reason("dpdk"
  "Duplicates declaration"
  testdata/p4_16_samples/psa-action-profile1.p4
  testdata/p4_16_samples/psa-action-profile2.p4
  testdata/p4_16_samples/psa-action-profile3.p4
  testdata/p4_16_samples/psa-action-profile4.p4
  testdata/p4_16_samples/psa-counter1.p4
  testdata/p4_16_samples/psa-counter2.p4
  testdata/p4_16_samples/psa-counter3.p4
  testdata/p4_16_samples/psa-counter4.p4
  testdata/p4_16_samples/psa-counter6.p4
  testdata/p4_16_samples/psa-custom-type-counter-index.p4
  testdata/p4_16_samples/psa-idle-timeout.p4
  testdata/p4_16_samples/psa-meter1.p4
  testdata/p4_16_samples/psa-meter3.p4
  testdata/p4_16_samples/psa-meter4.p4
  testdata/p4_16_samples/psa-meter5.p4
  testdata/p4_16_samples/psa-meter6.p4
  testdata/p4_16_samples/psa-test.p4
  testdata/p4_16_samples/psa-register1.p4
  )

p4c_add_xfail_reason("dpdk"
  "declaration not found"
  testdata/p4_16_samples/psa-action-selector1.p4
  testdata/p4_16_samples/psa-action-selector2.p4
  testdata/p4_16_samples/psa-action-selector3.p4
  testdata/p4_16_samples/psa-hash.p4
  testdata/p4_16_samples/psa-random.p4
  testdata/p4_16_samples/psa-register2.p4
  testdata/p4_16_samples/psa-register3.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unsupported parser loop"
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "not implemented"
  testdata/p4_16_samples/psa-example-register2-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unhandled type for"
  testdata/p4_16_samples/psa-meter7-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "error: AssignmentStatement"
  testdata/p4_16_samples/psa-multicast-basic-2-bmv2.p4
  testdata/p4_16_samples/psa-unicast-or-drop-bmv2.p4
  testdata/p4_16_samples/psa-example-parser-checksum.p4
  )

p4c_add_xfail_reason("dpdk"
  "Not Implemented"
  testdata/p4_16_samples/psa-register-read-write-2-bmv2.p4
  testdata/p4_16_samples/psa-end-of-ingress-test-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Error compiling"
  testdata/p4_16_samples/psa-register-complex-bmv2.p4
  testdata/p4_16_samples/psa-register-read-write-bmv2.p4
  testdata/p4_16_samples/psa-recirculate-no-meta-bmv2.p4
  )

