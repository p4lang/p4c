p4c_add_xfail_reason("dpdk"
  "use dpdk specific `dpdk_execute` method"
  testdata/p4_16_samples/psa-example-dpdk-meter-execute-err.p4
  testdata/p4_16_samples/psa-meter3.p4
  testdata/p4_16_samples/psa-meter7-bmv2.p4
  )
p4c_add_xfail_reason("dpdk"
  "Meter function execute not implemented, use dpdk_execute"
  testdata/p4_16_samples/psa-meter1.p4
  )
p4c_add_xfail_reason("dpdk"
  "Expected packet length argument for count method of indirect counter"
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
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
  "not implemented"
  testdata/p4_16_samples/psa-example-dpdk-byte-alignment_4.p4
  )

p4c_add_xfail_reason("dpdk"
  "cannot be the target of an assignment"
  testdata/p4_16_samples/psa-example-dpdk-byte-alignment_2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Not implemented"
  testdata/p4_16_samples/psa-random.p4
  )

p4c_add_xfail_reason("dpdk"
  "Error compiling"
  testdata/p4_16_samples/pna-dpdk-wrong-warning.p4
  testdata/p4_16_samples/pna-dpdk-invalid-hdr-warnings5.p4
  testdata/p4_16_samples/pna-dpdk-invalid-hdr-warnings6.p4
  )

p4c_add_xfail_reason("dpdk"
  "declaration not found"
  testdata/p4_16_samples/psa-dpdk-header-union-typedef.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unknown extern function"
  testdata/p4_16_samples/psa-example-digest-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "Unhandled declaration type"
  testdata/p4_16_samples/psa-test.p4
  )

p4c_add_xfail_reason("dpdk"
  "Direct counters and direct meters are unsupported for wildcard match"
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
  )

p4c_add_xfail_reason("dpdk"
  "can only be invoked from within action of ownertable"
  testdata/p4_16_samples/psa-example-dpdk-directmeter-err.p4
  testdata/p4_16_samples/pna-dpdk-direct-counter-err-3.p4
  )
p4c_add_xfail_reason("dpdk"
  "execute method not supported"
  testdata/p4_16_samples/psa-meter6.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected default action .* to have .* method call for .* extern instance"
   testdata/p4_16_samples/pna-dpdk-direct-counter-err-1.p4
   testdata/p4_16_samples/pna-dpdk-direct-meter-err-4.p4
   testdata/p4_16_samples/pna-dpdk-direct-meter-err-3.p4
  )

p4c_add_xfail_reason("dpdk"
  ".* method for different .* instances (.*) called within same action"
  testdata/p4_16_samples/pna-dpdk-direct-counter-err-4.p4
  )

p4c_add_xfail_reason("dpdk"
   "Learner table .* must have all exact match keys"
   testdata/p4_16_samples/pna-add-on-miss-err1.p4
   testdata/p4_16_samples/pna-dpdk-table-key-consolidation-learner-2.p4
  )
