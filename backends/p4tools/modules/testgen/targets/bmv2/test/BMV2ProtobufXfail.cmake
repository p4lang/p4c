# XFAILS: tests that *temporarily* fail
# ================================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.



####################################################################################################
# 2. P4Testgen Issues
####################################################################################################
# These are failures in P4Testgen that need to be fixed.

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown or unimplemented extern method: recirculate_preserving_field_list"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown or unimplemented extern method: extract"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Non-numeric, non-boolean member expression"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "with type .* is not a Constant"
  # Most of these come from varbits
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Cast failed"
  # push front can not handled tainted header validity.
  header-stack-ops-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "is trying to match on a tainted key set"
  invalid-hdr-warnings1.p4 # unimlemented feature (for select statement)
  issue692-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Only registers with bit or int types are currently supported"
  issue907-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Could not find type for"
  # Related to postorder(IR::Member* expression) in ParserUnroll,
  # and more specifically when member is passed to getTypeArray
)

####################################################################################################
# 3. WONTFIX
####################################################################################################
# These are failures that can not be solved by changing P4Testgen

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Assert/assume can not be executed under a tainted condition"
  # Assert/Assume error: assert/assume(false).
  bmv2_assert.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Computations are not supported in update_checksum"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Error compiling"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Checksum16.get is deprecated and not supported."
  issue841.p4 # Not supported
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown or unimplemented extern method: increment"
  issue1882-1-bmv2.p4  # user defined extern
  issue1882-bmv2.p4  # user defined extern
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown or unimplemented extern method: update"
  issue2664-bmv2.p4  # user defined extern
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown extern method count from type jnf_counter"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "Unknown or unimplemented extern method: fn_foo"
  issue3091.p4  # user defined extern
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "with type Type_Specialized is not a Type_Declaration"
  # Pipeline as a parameter of a switch, not a valid v1model program
  issue1304.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "is not a constant"
  # Using an uninitialized variable as a header stack index in the parser.
  parser-unroll-test10.p4
)

####################################################################################################
# 4. PARAMETERS NEEDED
####################################################################################################
# These tests require additional input parameters to compile properly.

# TODO: For these test we should add the --permissive flag.
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-protobuf"
  "The validity bit of .* is tainted"
  control-hs-index-test3.p4
  control-hs-index-test5.p4
  gauntlet_hdr_function_cast-bmv2.p4
  issue2345-1.p4
  issue2345-2.p4
  issue2345-multiple_dependencies.p4
  issue2345-with_nested_if.p4
  issue2345.p4
)
