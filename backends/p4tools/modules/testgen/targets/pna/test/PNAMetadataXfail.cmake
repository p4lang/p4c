# XFAILS: tests that *temporarily* fail
# ================================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: recirculate"
  pna-example-pass.p4
  pna-example-pass-3.p4
  pna-example-recirculate.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: get_hash"
  pna-dpdk-flatten-local-struct-decl.p4
  pna-dpdk-toeplitz-hash.p4
  pna-dpdk-toeplitz-hash-1.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: mirror_packet"
  pna-example-mirror-packet.p4
  pna-example-mirror-packet-1.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: count"
  pna-dpdk-direct-counter.p4
  pna-dpdk-direct-counter-learner.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: from_ipsec"
  pna-example-ipsec.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Unknown or unimplemented extern method: dpdk_execute"
  pna-dpdk-direct-meter-learner.p4
)

# Needs investigation.
p4tools_add_xfail_reason(
  "testgen-p4c-pna-metadata"
  "Internal error: Can not shift by a negative value."
)
