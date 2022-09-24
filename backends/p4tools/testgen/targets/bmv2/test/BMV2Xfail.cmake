# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================


####################################################################################################
# 1. P4C Toolchain Issues
# These are issues either with the P4 compiler or the behavioral model executing the code.
# These issues needed to be tracked and fixed in P4C.
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "simple_switch died with return code -6"
  # Assertion 'Default switch case should not be reachable' failed,
  # file '../../include/bm/bm_sim/actions.h' line '369'.
  issue1607-bmv2.p4
  bmv2_copy_headers.p4

  # terminate called after throwing an instance of 'std::out_of_range'
  # h.array[h.h.a].index
  # It turns out that h.h.a matters more than the size of the array
  bmv2_hs1.p4
  control-hs-index-test1.p4
  control-hs-index-test2.p4

  # terminate called after throwing an instance of 'boost::wrapexcept<std::range_error>'
  # Conversion from negative integer to an unsigned type results in undefined behaviour
  issue2726-bmv2.p4
  runtime-index-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "terminate called after throwing an instance of"
  # terminate called after throwing an instance of 'std::runtime_error'
  # in Json::Value::operator[](ArrayIndex)const: requires arrayValue
  control-hs-index-test6.p4
  issue3374.p4

  # terminate called after throwing an instance of 'std::runtime_error'
  # Type is not convertible to string
  control-hs-index-test3.p4
  parser-unroll-test1.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "bad json"
  invalid-hdr-warnings4.p4
  control-hs-index-test5.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Exception"
  #  Running simple_switch_CLI: Exception  Unexpected key field &
  match-on-exprs2-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Invalid reference to object of type"
  extract_for_header_union.p4
)

####################################################################################################
# 2. P4Testgen Issues
# These are failures in P4Testgen that need to be fixed.
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Computations are not supported in update_checksum"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Cast failed"
  # push front can not handled tainted header validity.
  header-stack-ops-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "is trying to match on a tainted key set"
  # unimlemented feature (for select statement)
  invalid-hdr-warnings1.p4
  issue692-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "differs|Expected ([0-9]+) packets on port ([0-9]+) got ([0-9]+)"
   # bug in parserunroll
  v1model-p4runtime-enumint-types1.p4
  issue281.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Only registers with bit or int types are currently supported"
  issue907-bmv2.p4
)

####################################################################################################
# 3. WONTFIX
# These are failures that can not be solved by changing P4Testgen
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Assert/assume can not be executed under a tainted condition"
  # Assert/Assume error: assert/assume(false).
  bmv2_assert.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "simple_switch died with return code -6"
  # Assert/Assume error: assert/assume(false).
  bmv2_assume.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Computations are not supported in update_checksum"
  issue1765-bmv2.p4
  issue1765-1-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "error: exit"
  # exit: Conditional execution in actions unsupported on this target.
  issue2359.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Error compiling"
  # IfStatement: not supported within a deparser on this target.
  issue887.p4
)
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Checksum16.get is deprecated and not supported."
  # Not supported
  issue841.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Unknown or unimplemented extern method: increment"
  # user defined externs
  issue1882-1-bmv2.p4
  issue1882-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Unknown or unimplemented extern method: update"
  issue2664-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Unknown extern method count from type jnf_counter"
  # user defined extern
  issue1193-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Unknown or unimplemented extern method: fn_foo"
  # user defined extern
  issue3091.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "The BMV2 architecture requires 6 pipes"
  # All of these tests are not valid bmv2 programs but import v1model.
  # TODO: Fix up these programs so that they pass.
  logging.p4
  issue584-1.p4
  issue1806.p4
  issue2104.p4
  issue2105.p4
  issue2890.p4
  noMatch.p4
  pred.p4
  pred1.p4
  pred2.p4
  register-serenum.p4
  table-key-serenum.p4
  issue3531.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "BMv2 target only supports headers with fields totaling a multiple of 8 bits"
  custom-type-restricted-fields.p4
  issue3225.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "with type Type_Specialized is not a Type_Declaration"
  # Pipeline as a parameter of a switch, not a valid v1model program
  issue1304.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Program is not supported by this target"
  issue986-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "Program can not be implemented on this target"
  # The error appears because the table calls itself:
  # apply {
  #  simple_table.apply();
  #  simple_table.apply();
  # }
  # if you remove one of the calls, then the test will pass, but at the end there will be an error:
  # Transmitting packet of size 1333 out of port 0
  issue2344.p4
)


p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "error.NoError: unsupported exact key expression"
  issue1062-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "expected type to be a struct"
  issue3394.p4
)

####################################################################################################
# 4. PARAMETERS NEEDED
####################################################################################################
# These tests require additional input parameters to compile properly.

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "uninitialized: next field read"
  # error: parsedHdr.hstack.next uninitialized: next field read
  # next not implemented in p4c/backends/bmv2/common/expression.cpp line 367
  next-def-use.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
  "headers: the type should be a struct of headers, stacks, or unions"
  parser-unroll-test3.p4
  parser-unroll-test5.p4
  parser-unroll-test4.p4
)

# TODO: For these test we should add the --permissive flag.
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2"
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
