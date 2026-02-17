# XFAILS: tests that *temporarily* fail
# =====================================
#
# Xfails are _temporary_ failures: the tests should work but we haven't fixed
# the tools yet.

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method:"
  # mirror.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: emit"
  # Resubmit.emit
  # tna_resubmit.p4
  # empty_header_stack.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: set"
  # parser_counter_10.p4
  # parser_counter_7.p4
  # parser_counter_stack_1.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: enqueue"
  # RegisterAction.enqueue
  # t2na_fifo.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: execute"
  # MinMaxAction.execute
  # t2na_ghost.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: clear"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Unknown or unimplemented extern method: write"
  # RegisterAction.write
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "writing to a structure is not supported"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "error: Power worst case estimated budget"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "is trying to match on a tainted key set"
  # parser_multi_write_checksum_verify_5.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Expected packet was not received on device .*"
  # Our packet model is incomplete.
  # We do not model the case where we append metadata but do not parse it.
  # tna_register.p4
  # tna_alpmV2.p4
  # Most likely a compiler bug
  # tna_extract_emit_3.p4
  # tna_action_profile_1.p4
  # Meter is not fully implemented.
  # large_counter_meter.p4
  # TODO:
  # tna_multicast.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "exceeded the maximum number of permitted guard violations for this run"
  # t2na_emulation.p4
  # too_many_pov_bits.p4
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "P4Testgen Bug: Unable to find var .* in the symbolic environment"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "A packet was received on device .*"
)

# This p4 is supposed to compile with --no-bf-rt-schema option, not sure how to run it though.
p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "is used for multiple Register objects in the P4Info message"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Match type dleft_hash not implemented for table keys"
  # hwlrn.p4
)

# This is out of our control.
p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Table Add failed .* Not enough space"
)

p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Failed to subscribe to server at %s"
  # varbit_four_options.p4
)

# These are compilation bugs we have no control over.
p4tools_add_xfail_reason(
  "testgen-tofino2-ptf"
  "Compiler failed"
)
