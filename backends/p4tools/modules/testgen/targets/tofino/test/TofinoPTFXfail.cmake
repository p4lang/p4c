# XFAILS: tests that *temporarily* fail
# =====================================
#
# Xfails are _temporary_ failures: the tests should work but we haven't fixed
# the tools yet.

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "cast"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Not a valid state variable"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unable to resolve extraction source"
  # simple_l3_mcast.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "not padded to be byte-aligned in"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "writing to a structure"
  # These tests fail because the nestedStructs pass does not unroll assignments
  # to a structure that has been returned from an extern
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Table key name not supported"
  # static_entries2.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unable to find var"
  # Need to convert a path expression reference into a struct expression.
)

# Valid tna program fails in MidEnd_32_BFN::RewriteEgressIntrinsicMetadataHeader pass
p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Program not supported by target device and architecture"
  # test_config_2_multiple_parsers.p4
  # test_config_3_unused_parsers.p4
  # test_config_11_multi_pipe_multi_parsers.p4
  # tna_multi_prsr_programs_multi_pipes.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "StackOutOfBounds will be triggered"
  # parser_loop_3.p4
  # parser_loop_4.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "error: This program violates action constraints imposed by Tofino"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Saturating arithmetic operators"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unknown or unimplemented extern method: is_zero"
  # tcp_option_mss.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unknown or unimplemented extern method: write"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Cannot cast implicitly type"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unknown or unimplemented extern method: emit"
  # tna_resubmit.p4
  # mirror.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unknown or unimplemented extern method: execute"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unknown or unimplemented extern method: set"
  # parser_counter_6.p4
  # parser_counter_7.p4
  # parser_counter_8.p4
  # parser_counter_9.p4
  # parser_counter_10.p4
  # parser_match_17.p4
  # parser_counter_12.p4
  # parser_counter_11.p4
  # ipv6_tlv.p4
  # parser_loop_1.p4
  # parser_loop_2.p4
  # tcp_option_mss_4_byte_chunks.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Found 2 duplicate name"
  # multiple_apply2.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "A packet was received on device"
  # Potentially a bug in bf-p4c.
  # tna_action_profile_1.p4
  # We are not modelling short packets correctly.
  # TODO
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Expected packet was not received"
  # Our packet model is incomplete.
  # We do not model the case where we append metadata but do not parse it.
  # tna_register.p4
  # parse_srv6_fast.p4
  # This is likely a bug in bf-p4c
  # header_stack_strided_alloc2.p4
  # action_profile.p4
  # multithread_pipe_lock.p4
  # snapshot.p4
  # Multicast is not implemented
  # tna_multicast.p4
  # These tests currently fail because of a bug in bf-p4c.
  # symmetric_hash.p4
  # This looks like a problem with the @flexible annotation.
  # meter_dest_16_32_flexible.p4
  # Missing implementation for pa_alias
  # mau-meter.cpp:233 (calculate_output): error -2 thrown.
  # large_counter_meter.p4
  # TODO
  # tna_alpmV2.p4
  # varbit_four_options.p4
)

# New tests added
p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Unimplemented compiler support"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Please rewrite the action to be a single stage action"
)

# These are tests that currently fail.
p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler failed"
  # mirror_5.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "failed command verifier"
  # simple_l3_nexthop_ipv6_options.p4
)

# Trying to compile a t2na program.
p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "error: AssignmentStatement"
  # t2na_tm_stress.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler Bug: : Operand .* of instruction .* operating on container .* must be a PHV."
  # deparse-zero-clustering.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler Bug: For Tofino, Index of the header stack ."
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler Bug: Flexible packing bug found"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler Bug: Could not find field"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "error: mirror.emit"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Compiler Bug: Inconsistent parser write semantic on .*"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "error: Power worst case estimated budget"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "is trying to match on a tainted key set"
  # lookahead1.p4
  switchml.p4
  # parser_multi_write_checksum_verify_5.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "error: Size of learning quanta is .* bytes, greater than the maximum allowed .* bytes."
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "longest path through parser (.*) exceeds maximum parse depth (.*)"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Tofino requires byte-aligned headers"
  # tagalong_mdinit_switch.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Ensure that the atcam_partition_index and exact match key name match"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "exceeded the maximum number of permitted guard violations for this run"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "table .* should not have empty const entries list"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Internal error: Attempt to assign a negative value to an unsigned type."
  # The parser of the interpreter hits an infinite loop.
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "PTF runner:Error when starting model & switchd"
  # Switchd fails to start with context json processing error.
  # forensics.p4
)

# Most likely a model bug
p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "BfruntimeReadWriteRpcException"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "The validity bit of .* is tainted"
  tna_simple_switch.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "The varbit expression of .* is tainted"
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "We can not predict how much this call will advance the parser cursor"
  # varbit_in_middle.p4
  # simple_l3_csum_varbit_v2.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Failed to subscribe to server at %s"
  # (check_extract_constraints): error -2 thrown
)

p4tools_add_xfail_reason(
  "testgen-tofino-ptf"
  "Table Add failed .* Not enough space"
)
