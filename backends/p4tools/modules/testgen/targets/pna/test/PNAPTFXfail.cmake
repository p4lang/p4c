# XFAILS: tests that *temporarily* fail
# ================================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "P4Info is missing these required resources: Tables Actions."
)

p4tools_add_xfail_reason(
  "testgen-p4c-pna-ptf"
  "Unknown or unimplemented extern method: .*[.]dpdk_execute"
  pna-dpdk-direct-meter-learner.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: .*[.]get_hash"
    pna-dpdk-flatten-local-struct-decl.p4
    pna-dpdk-toeplitz-hash-1.p4
    pna-dpdk-toeplitz-hash.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: .*[.]from_ipsec"
    pna-example-ipsec.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unexpected error."
    pna-dpdk-parser-state-err.p4
    pna-elim-hdr-copy-dpdk.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    ""
    pna-elim-hdr-copy-dpdk.p4
    pna-too-big-label-name-dpdk.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "One or more write operations failed."
    pna-dpdk-table-key-use-annon.p4
)

# pna-dpdk-parser-wrong-arith: generated P4Info can miss mandatory resources
# (Tables/Actions), so SetForwardingPipelineConfig is rejected.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "P4Info is missing these required resources: Tables Actions."
    pna-dpdk-parser-wrong-arith.p4
)

# issue4796: DPDK pipeline load fails in bf_pal_device_add with a struct-type
# registration error during pipeline enable.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Struct type registration error."
    issue4796.p4
)

# pna-dpdk-large-constants: DPDK pipeline instruction configuration fails,
# then device add fails with the same Unexpected error.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Pipeline instructions configuration error."
    pna-dpdk-large-constants.p4
)

# pna-dpdk_128bit_odd_size: control-plane writes can crash DPDK pipeline code
# in pipe_mgr_dpdk_encode_action_data (buffer overflow / abort), which then
# disconnects P4Runtime.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "buffer overflow detected)"
    pna-dpdk_128bit_odd_size.p4
)
