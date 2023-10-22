# XFAILS: tests that *temporarily* fail
# ================================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.

p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "P4Info is missing these required resources: Tables Actions."
        pna-dpdk-bvec_union.p4
        pna-dpdk-header-union-stack.p4
        pna-dpdk-header-union-stack1.p4
        pna-dpdk-union-bmv2.p4
)

p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "Unknown or unimplemented extern method: dpdk_execute"
        pna-dpdk-direct-meter-learner.p4
)

p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "Unknown or unimplemented extern method: get_hash"
        pna-dpdk-flatten-local-struct-decl.p4
        pna-dpdk-toeplitz-hash-1.p4
        pna-dpdk-toeplitz-hash.p4
)

p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "Unknown or unimplemented extern method: from_ipsec"
        pna-example-ipsec.p4
)

p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "terminate called after throwing an instance of \'Util::CompilerBug\'"
        pna-dpdk-header-union-stack2.p4
        pna-dpdk-invalid-hdr-warnings5.p4
        pna-dpdk-invalid-hdr-warnings6.p4
        pna-dpdk-wrong-warning.p4
)


p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "Unexpected error."
        pna-dpdk-parser-state-err.p4
        pna-elim-hdr-copy-dpdk.p4
        pna-too-big-label-name-dpdk.p4
)

p4tools_add_xfail_reason(
        "testgen-p4c-pna-ptf"
        "One or more write operations failed."
        pna-dpdk-table-key-use-annon.p4
)
