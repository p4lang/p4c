# XFAILS: tests that *temporarily* fail
# =====================================
#
# Xfails are _temporary_ failures: the tests should work but we haven't fixed
# the tools yet.

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Match type range not implemented"
  # tna_range_match.p4
  # tna_field_slice.p4
  # t2na_uni_dim_scale_base.p4
  # varbit_four_options.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Match type dleft_hash not implemented"
  # hwlrn.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method:"
  # mirror.p4
  # t2na_ghost.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: emit"
  # Resubmit.emit
  # tna_resubmit.p4
  # empty_header_stack.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: set"
  # parser_counter_10.p4
  # parser_counter_7.p4
  # parser_counter_stack_1.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: execute"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: clear"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: write"
  # RegisterAction.write
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Unknown or unimplemented extern method: enqueue"
  # RegisterAction.enqueue
  # t2na_fifo.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "warning: .*: Table key name not supported. Replacing .* with .*."
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Not a valid state variable"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "writing to a structure is not supported"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "unhandled stage table type"
  # tna_proxy_hash.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Table .* has no stage tables"
  # bfrt_alpm_perf.p4
  # tna_lpm_match.p4
  # tna_alpm.p4
  # tna_alpmV2.p4
  # tna_ipv4_alpm.p4
  # tna_ipv6_alpm.p4
  # tna_ternary_match.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "error -2 thrown"
  # tna_meter_bytecount_adjust.p4
  # tna_meter_lpf_wred.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "terminate called after throwing an instance of"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "is trying to match on a tainted key set"
  switch_tofino2_y1_mod.p4
  # parser_multi_write_checksum_verify_5.p4
)

# Looks like a compiler/test_harness bug
p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Clot slices must be used in order"
  # tna_extract_emit_3.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "default_action_handle .* not found in table .*"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "must specify match fields before action"
  # tna_checksum.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "exceeded the maximum number of permitted guard violations for this run"
  # t2na_emulation.p4
  # too_many_pov_bits.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "shorter than expected"
  # Strange behavior with packets that are too short when using the STF model.
  tna_extract_emit_8.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "1 expected packet on port .* not seen"
  # tna_multicast.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "invalid vpn .* in write_sram for logical table .*"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "no field .* related to table .*"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "mismatch from expected"
  # Our packet model is incomplete.
  # We do not model the case where we append metadata but do not parse it.
  # tna_register.p4
  # Meter is not fully implemented.
  # large_counter_meter.p4
  # TODO
)

# This p4 is supposed to compile with --no-bf-rt-schema option, not sure how to run it though.
p4tools_add_xfail_reason(
  "testgen-tofino2"
  "is used for multiple Register objects in the P4Info message"
)

p4tools_add_xfail_reason(
  "testgen-tofino2"
  "duplicate value for .*"
  # misc1.p4
)

# These are tests that currently fail.
p4tools_add_xfail_reason(
  "testgen-tofino2"
  "Compiler failed"
)
