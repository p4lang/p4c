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
  "Expected atleast 2 arguments"
  testdata/p4_16_samples/psa-meter3.p4
  testdata/p4_16_samples/psa-meter7-bmv2.p4
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
  "Only one LPM match field is permitted per table"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err1.p4
  )

p4c_add_xfail_reason("dpdk"
  "Non 'exact' match kind not permitted"
  testdata/p4_16_samples/psa-dpdk-lpm-match-err2.p4
  )

p4c_add_xfail_reason("dpdk"
  "mirror_packet cannot be used in the"
  testdata/p4_16_samples/pna-example-mirror-packet-ctxt-error.p4
  testdata/p4_16_samples/pna-example-mirror-packet-ctxt-error1.p4
  )

p4c_add_xfail_reason("dpdk"
  "Mirror session ID 0 is reserved for use by Architecture"
  testdata/p4_16_samples/pna-example-mirror-packet-error1.p4
  )

p4c_add_xfail_reason("dpdk"
  "argument used for directionless parameter .* must be a compile-time constant"
  testdata/p4_16_samples/pna-example-mirror-packet-error2.p4
  )

p4c_add_xfail_reason("dpdk"
  "No argument supplied for parameter"
  testdata/p4_16_samples/pna-example-mirror-packet-error3.p4
  )

p4c_add_xfail_reason("dpdk"
  "must only be called from within an action"
  testdata/p4_16_samples/pna-add-on-miss-err.p4
  testdata/p4_16_samples/pna-example-tcp-connection-tracking-err-1.p4
  testdata/p4_16_samples/pna-example-tcp-connection-tracking-err.p4
  testdata/p4_16_samples/pna-dpdk-direct-meter-err-2.p4
  testdata/p4_16_samples/pna-dpdk-direct-counter-err-2.p4
  )

p4c_add_xfail_reason("dpdk"
  "add_entry is not called from a default action"
  testdata/p4_16_samples/pna-example-addhit.p4
  )

p4c_add_xfail_reason("dpdk"
  "add_entry action cannot be default action"
   testdata/p4_16_samples/pna-example-miss.p4
   )

p4c_add_xfail_reason("dpdk"
  "must be a compile-time constant"
  testdata/p4_16_samples/psa-example-mask-range-err.p4
  )

p4c_add_xfail_reason("dpdk"
  "implementation property cannot co-exist with direct counter"
  testdata/p4_16_samples/psa-example-dpdk-directmeter-1.p4
  testdata/p4_16_samples/pna-dpdk-direct-counter-err.p4
  testdata/p4_16_samples/pna-dpdk-direct-meter-err.p4
  )

p4c_add_xfail_reason("dpdk"
  "Direct counters and direct meters are unsupported for wildcard match"
  testdata/p4_16_samples/psa-example-counters-bmv2.p4
  testdata/p4_16_samples/pna-dpdk-direct-meter-err-1.p4
  )

p4c_add_xfail_reason("dpdk"
  "can only be invoked from within action of ownertable"
  testdata/p4_16_samples/psa-example-dpdk-directmeter-err.p4
  testdata/p4_16_samples/pna-dpdk-direct-counter-err-3.p4
  testdata/p4_16_samples/psa-meter6.p4
  )

p4c_add_xfail_reason("dpdk"
  "Expected default action .* to have .* method call for .* extern instance"
   testdata/p4_16_samples/pna-dpdk-direct-counter-err-1.p4
   testdata/p4_16_samples/pna-dpdk-direct-meter-err-4.p4
  )

p4c_add_xfail_reason("dpdk"
  ".* method for different .* instances (.*) called within same action"
  testdata/p4_16_samples/pna-dpdk-direct-counter-err-4.p4
  testdata/p4_16_samples/pna-dpdk-direct-meter-err-3.p4
  )

p4c_add_xfail_reason("dpdk"
   "Learner table .* must have all exact match keys"
   testdata/p4_16_samples/pna-add-on-miss-err1.p4
   testdata/p4_16_samples/pna-dpdk-table-key-consolidation-learner-2.p4
  )
