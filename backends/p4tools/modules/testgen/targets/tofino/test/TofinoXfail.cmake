# XFAILS: tests that *temporarily* fail
# =====================================
#
# Xfails are _temporary_ failures: the tests should work but we haven't fixed
# the tools yet.

p4tools_add_xfail_reason(
  "testgen-tofino"
  "cast"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Not a valid state variable"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unimplemented for LPM FieldMatch"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unable to resolve extraction source"
  simple_l3_mcast.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "not padded to be byte-aligned in"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "writing to a structure"
  # These tests fail because the nestedStructs pass does not unroll assignments
  # to a structure that has been returned from an extern
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Table key name not supported"
  static_entries2.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unable to find var"
  # Need to convert a path expression reference into a struct expression.
)

# Valid tna program fails in MidEnd_32_BFN::RewriteEgressIntrinsicMetadataHeader pass
p4tools_add_xfail_reason(
  "testgen-tofino"
  "Program not supported by target device and architecture"
  test_config_2_multiple_parsers.p4
  test_config_3_unused_parsers.p4
  test_config_11_multi_pipe_multi_parsers.p4
  tna_multi_prsr_programs_multi_pipes.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "StackOutOfBounds will be triggered"
  parser_loop_3.p4
  parser_loop_4.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "error: This program violates action constraints imposed by Tofino"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Saturating arithmetic operators"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unknown or unimplemented extern method: is_zero"
  tcp_option_mss.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unknown or unimplemented extern method: write"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Cannot cast implicitly type"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unknown or unimplemented extern method: emit"
  tna_resubmit.p4
  mirror.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unknown or unimplemented extern method: execute"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Cast failed"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unknown or unimplemented extern method: set"
  parser_counter_6.p4
  parser_counter_7.p4
  parser_counter_8.p4
  parser_counter_9.p4
  parser_counter_10.p4
  parser_match_17.p4
  parser_counter_12.p4
  parser_counter_11.p4
  ipv6_tlv.p4
  parser_loop_1.p4
  parser_loop_2.p4
  tcp_option_mss_4_byte_chunks.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Found 2 duplicate name"
  multiple_apply2.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "mismatch from expected"
  # Our packet model is incomplete.
  # We do not model the case where we append metadata but do not parse it.
  # tna_register.p4
  # Known issue
  # These tests currently fail because of a bug in bf-p4c.
  snapshot.p4
  # symmetric_hash.p4
  # This looks like a problem with the @flexible annotation.
  # mau-meter.cpp:233 (calculate_output): error -2 thrown.
  large_counter_meter.p4
  # pa_alias not implemented.
  # TODO
  meter_dest_16_32_flexible.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Malformed packet data"
  # This looks like a problem with the @flexible annotation.
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "1 expected packet on port .* not seen"
  header_stack_strided_alloc2.p4
  parse_srv6_fast.p4
  # STF seems to handle these wild tests incorrectly and fails on every generated testcase.
  # Might be a bug in the tofino STF harness...
  static_entries_over_multiple_stages.p4
  # Multicast is not implemented
  tna_multicast.p4
  # Missing implementation for pa_alias
  # TODO:
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "longer than expected"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "shorter than expected"
  # Strange behavior with packets that are too short when using the STF model.
  tna_extract_emit_8.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "no argument .* for .*"
  # test_harness does not process multiple arguments for overriding default actions.
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Match type range not implemented"
  color_aware_meter.p4
  tna_range_match.p4
  tna_field_slice.p4
  tagalong_mdinit_switch.p4
  varbit_four_options.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "warning: .*: Table key name not supported. Replacing .* with .*."
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Table .* has no stage tables"
  atcam_match_wide1.p4
  atcam_match1.p4
  atcam_match2.p4
  atcam_match3.p4
  atcam_match4.p4
  atcam_match5.p4
  bfrt_alpm_perf.p4
  tna_alpm.p4
  tna_alpmV2.p4
  tna_lpm_match.p4
  alpm.p4
  alpm_2.p4
  alpm_4.p4
  atcam_match_extern.p4
  tna_ipv4_alpm.p4
  tna_ipv6_alpm.p4
  tna_ternary_match.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "no field .* related to table .*"
)

# New tests added
p4tools_add_xfail_reason(
  "testgen-tofino"
  "Unimplemented compiler support"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "unhandled stage table type"
  proxy_hash.p4
  tna_proxy_hash.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Please rewrite the action to be a single stage action"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "error -2 thrown"
  # https://jira.devtools.intel.com/browse/MODEL-1151
  meters_adjust_byte_count.p4
  tna_meter_bytecount_adjust.p4
  # TODO - Check if this is a new issue - test harness crashed.
  switch_tofino_x2.p4
  counter_meter_test.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "terminate called after throwing an instance of"
  test_compiler_macro_defs.p4
)

# These are tests that currently fail.
p4tools_add_xfail_reason(
  "testgen-tofino"
  "Compiler failed"
  mirror_5.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "failed command verifier"
  simple_l3_nexthop_ipv6_options.p4
)

# Trying to compile a t2na program.
p4tools_add_xfail_reason(
  "testgen-tofino"
  "error: AssignmentStatement"
  t2na_tm_stress.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Compiler Bug: : Operand .* of instruction .* operating on container .* must be a PHV."
  deparse-zero-clustering.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Compiler Bug: For Tofino, Index of the header stack ."
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "error: mirror.emit"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "error: Power worst case estimated budget"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "is trying to match on a tainted key set"
  lookahead1.p4
  switchml.p4
  parser_multi_write_checksum_verify_5.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "error: Size of learning quanta is .* bytes, greater than the maximum allowed .* bytes."
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "invalid vpn .* in write_sram for logical table .*"
  snapshot_all_stages.p4
  snapshot_all_stages_egress.p4
  forensics.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "longest path through parser (.*) exceeds maximum parse depth (.*)"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Compiler Bug: Flexible packing bug found"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "exceeded the maximum number of permitted guard violations for this run"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "table .* should not have empty const entries list"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "default_action_handle .* not found in table ipv4_lpm"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Internal error: Attempt to assign a negative value to an unsigned type."
  # The parser of the interpreter hits an infinite loop.
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "must specify match fields before action"
  tna_checksum.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "Tofino requires byte-aligned headers"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "The validity bit of .* is tainted. Tainted emit calls can not be mitigated"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "The varbit expression of .* is tainted"
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "We can not predict how much this call will advance the parser cursor"
  varbit_in_middle.p4
  simple_l3_csum_varbit_v2.p4
)

# Issue with the Tofino test harness.
p4tools_add_xfail_reason(
  "testgen-tofino"
  "duplicate value for .*"
  tcam_no_versioning.p4
  misc1.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino"
  "invalid vpn .* in write_sram for logical table"
)

# TODO - New bug, need to debug.
p4tools_add_xfail_reason(
  "testgen-tofino"
  "no argument .*"
)
