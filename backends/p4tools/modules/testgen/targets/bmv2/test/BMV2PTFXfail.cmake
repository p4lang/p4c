# XFAILS: tests that *temporarily* fail
# =====================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.

# TODO: For these test we should add the --permissive flag.
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "The validity bit of .* is tainted"
    up4.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Internal error"
  #variable 'action.action_args' not found
  issue297-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  ""
  # Assertion 'Default switch case should not be reachable' failed,
  # file '../../include/bm/bm_sim/actions.h' line '369'.
  issue1607-bmv2.p4

  # terminate called after throwing an instance of 'std::runtime_error'
  # in Json::Value::operator[](ArrayIndex)const: requires arrayValue
  issue3374.p4

  # terminate called after throwing an instance of 'std::runtime_error'
  # Type is not convertible to string
  control-hs-index-test3.p4
  parser-unroll-test1.p4

  # Address family not supported by protocol
  bmv2_read_write.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "expected type to be a struct"
  issue3394.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "expected a struct"
  hashing-non-tuple-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Expected packet"
  bmv2_lookahead_2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "no port"
  issue2726-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Checksum16.get is deprecated and not supported."
  # Not supported
  issue841.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "BMv2 target only supports headers with fields totaling a multiple of 8 bits"
  custom-type-restricted-fields.p4
  issue3225.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "out of port"
  issue461-bmv2.p4
  issue774-4-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "expected no packets"
  issue1897-bmv2.p4
  issue949.p4
  table-entries-ser-enum-bmv2.p4
  union-valid-bmv2.p4
  issue-2123.p4
  table-entries-valid-bmv2.p4
  gauntlet_side_effects_in_mux-bmv2.p4
  gauntlet_mux_typecasting-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "is trying to match on a tainted key set"
  # unimlemented feature (for select statement)
  invalid-hdr-warnings1.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Assert/assume can not be executed under a tainted condition"
  # Assert/Assume error: assert/assume(false).
  bmv2_assert.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "headers: the type should be a struct of headers, stacks, or unions"
  parser-unroll-test3.p4
  parser-unroll-test5.p4
  parser-unroll-test4.p4
)
