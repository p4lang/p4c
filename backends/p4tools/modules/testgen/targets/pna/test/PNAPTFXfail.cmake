# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# XFAILS: tests that *temporarily* fail
# ================================================
# These cover Testgen, compiler, and pinned IPDK runtime limitations. Match the
# observed diagnostic so unrelated failures are not silently accepted.

# Testgen: no model for recirculation. Generation stops before packet testing.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: recirculate"
    pna-example-pass.p4
    pna-example-pass-3.p4
    pna-example-recirculate.p4
)

# Testgen: no model for packet mirroring.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: mirror_packet"
    pna-example-mirror-packet.p4
    pna-example-mirror-packet-1.p4
)

# Testgen: no model for these direct-counter calls.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: .*[.]count"
    pna-dpdk-direct-counter.p4
    pna-dpdk-direct-counter-learner.p4
)

# Testgen: no model for the DPDK direct-meter operation.
p4tools_add_xfail_reason(
  "testgen-p4c-pna-ptf"
  "Unknown or unimplemented extern method: .*[.]dpdk_execute"
  pna-dpdk-direct-meter-learner.p4
)

# Testgen: no model for these hash operations.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: .*[.]get_hash"
    pna-dpdk-flatten-local-struct-decl.p4
    pna-dpdk-toeplitz-hash-1.p4
    pna-dpdk-toeplitz-hash.p4
)

# Testgen: no model for the IPsec from_ipsec operation.
# Missing extern models do not establish whether the runtime supports them.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Unknown or unimplemented extern method: .*[.]from_ipsec"
    pna-example-ipsec.p4
)

# Compiler/runtime compatibility: the compiler emits an empty structure that
# SWX rejects when loading the pipeline, before any generated packets are sent.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Struct type registration error."
    issue4796.p4
)

# Compiler/runtime compatibility: SWX rejects the compiled pipeline instructions.
# The direction and header-copy programs emit comparisons with a constant as
# the first operand. The other cases need further isolation of operand and
# label restrictions. These failures happen before generated table entries or
# packets are used; the generic error does not identify the exact fix.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Pipeline instructions configuration error."
    pna-dpdk-large-constants.p4
    pna-dpdk-parser-state-err.p4
    pna-direction-main-parser-err.p4
    pna-direction.p4
    pna-elim-hdr-copy-dpdk.p4
    pna-too-big-label-name-dpdk.p4
)

# IPDK runtime: action arguments wider than 64 bits overflow a uint64_t buffer
# in pipe_mgr_dpdk_encode_action_data. The indirect-action encoder has the same
# bug. A separate upstream patch passed encoding and packet tests, but the
# pinned runtime still needs that fix.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "buffer overflow detected"
    pna-dpdk_128bit_odd_size.p4
)

# Testgen: SelectByDirection in shared_expr_stepper.cpp queues the same state
# for both directions instead of using hostToNetState for the second branch.
# Both opposing conditions are added to one state, leaving no feasible path.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "No PTF tests generated"
    issue2900.p4
)

# Testgen assumes it can prefill add-on-miss tables through the control plane.
# This target does not support that optional capability. Testing learned hits
# requires packet sequences that create entries through the data plane.
# PNA permits targets to reject control-plane modification of these tables,
# so the rejected INSERTs are not evidence of a runtime bug. Testgen must
# respect add_on_miss and track learned state; shared_expr_stepper.cpp currently
# treats add_entry as a successful no-op.
# pna-dpdk-add_on_miss0 is not listed: its ten generated cases passed in CI
# without reaching the unsupported inserts. That does not cover all its paths.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Error adding table entry with table_name: .*table_type: 2048"
    pna-add-on-miss.p4
    pna-add_on_miss_action_name.p4
    pna-dpdk-add_on_miss1.p4
    pna-dpdk-table-key-consolidation-learner-1.p4
    pna-dpdk-table-key-consolidation-learner-3.p4
    pna-dpdk-table-key-consolidation-learner-4.p4
    pna-dpdk-table-key-consolidation-learner-5.p4
    pna-dpdk-table-key-consolidation-learner-6.p4
    pna-dpdk-table-key-consolidation-learner-7.p4
    pna-dpdk-table-key-use-annon.p4
)

# Compiler: crashes with a null expression while compiling the original P4
# programs for packet testing. These are not generated-packet mismatches.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Compiler Bug: .*Null expr"
    pna-dpdk-header-union-stack2.p4
    pna-dpdk-invalid-hdr-warnings5.p4
    pna-dpdk-invalid-hdr-warnings6.p4
)
# Compiler: fails to resolve a header type while compiling the original program.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Could not find type of <Type_Header>"
    pna-dpdk-wrong-warning.p4
)

# Compiler/runtime compatibility: SWX rejects the emitted action instructions
# during pipeline loading. The exact rejected instructions and required fixes
# still need isolation; generated packets have not run at this point.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Action configuration error."
    pna-ipv6-actions.p4
    pna-mux-dismantle.p4
)

# Stratum control plane: tdi_table_manager.cc explicitly rejects P4Runtime
# optional matches. Testgen's request uses the match kind declared by the P4
# program; the missing support is in the control-plane translation.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Invalid or unsupported match key: field_id: 2 optional"
    pna-example-dpdk-optional.p4
)

# IPDK runtime: insertion succeeds, but deletion fails during cleanup. Later
# cases can then report duplicate entries. In dal_mat.c, insertion uses
# target_table_name while deletion uses name; these differ for nested tables
# such as tunnel_encap_set_tunnel_encap. This points to a runtime name mismatch,
# not a Python cleanup bug. A patch still needs packet-level validation.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Error deleting table entry with table_name: pipe.main_control.tunnel_encap.set_tunnel_encap"
    pna-example-tunnel.p4
)

# Compiler: relational comparisons on the left of || have incorrect
# short-circuit lowering. The emitted jumps implement the wrong condition,
# causing the packet result to differ from the P4 program's expected behavior.
p4tools_add_xfail_reason(
    "testgen-p4c-pna-ptf"
    "Expected packet was not received on device 0, port 0."
    pna-dpdk-leq-lor.p4
)
