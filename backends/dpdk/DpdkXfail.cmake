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
  "All table keys together with holes in the underlying structure should fit in 64 bytes"
   testdata/p4_16_samples/psa-dpdk-table-key-error.p4
   testdata/p4_16_samples/psa-dpdk-table-key-error-1.p4
   )

p4c_add_xfail_reason("dpdk"
  "must only be called from within an action"
  testdata/p4_16_samples/pna-add-on-miss-err.p4
  testdata/p4_16_samples/pna-example-tcp-connection-tracking-err-1.p4
  testdata/p4_16_samples/pna-example-tcp-connection-tracking-err.p4
  )
