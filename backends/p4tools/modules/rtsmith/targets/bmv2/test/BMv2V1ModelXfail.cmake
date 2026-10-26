# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify.
p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  ""
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Compiler Bug|Unimplemented compiler support"
  issue1304.p4 # Cast failed: Pipeline<my_packet, my_metadata> with type Type_Specialized is not a
               # Type_Declaration.
  header-stack-ops-bmv2.p4 # Unknown method member expression: hdr_0.h2; of type header h2_t
  issue4739.p4 # type ForStatement not implemented in the core stepper
  loop-3-clause-tricky2.p4 # Node for (i_0/i = 0; i_0/i < 7; i_0/i = i_0/i + 3) { n_0/n = n_0/n + (bit<8>)i_0/i }
                           # of type ForStatement not implemented in the core stepper.
)

# These are custom externs we do not implement.
p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Unknown or unimplemented extern method: .*"
  issue1882-bmv2.p4 # Unknown or unimplemented extern method: extr.increment
  issue1882-1-bmv2.p4 # Unknown or unimplemented extern method: extr.increment
  issue2664-bmv2.p4 # Unknown or unimplemented extern method: ipv4_checksum.update
  issue1193-bmv2.p4 # Unknown or unimplemented extern method: count
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Unable to find var .* in the symbolic environment"
  union-valid-bmv2.p4 # Unable to find var h.u.*valid; in the symbolic environment.
  issue3091.p4 # WONTFIX Unable to find var ternary; in the symbolic environment.
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "expected a header or header union stack"
  issue4057.p4
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model" "Only constants are supported"
  parser-unroll-test10.p4 # Value meta.hs_next_index; is not a constant. Only constants are
                          # supported as part of a state variable.
                          # hdr.hs[meta.hs_next_index].setValid();
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Unsupported type argument for Value Set"
  pvs-nested-struct.p4
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Parser state .* was already visited. We currently do not support parser loops."
  issue2314.p4
  invalid-hdr-warnings1.p4
  parser-unroll-test9.p4
  parser-unroll-issue3537.p4
  parser-unroll-issue3537-1.p4
  issue281.p4
  fabric.p4
)

p4tools_add_xfail_reason(
  "rtsmith-checker-bmv2-v1model"
  "Compiler Bug: Unexpected width 0"
  # These fail because the key is an enum, which has a 0 width value.
  issue1062-1-bmv2.p4
  issue2283_1-bmv2.p4
)
