# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================


####################################################################################################
# 1. P4C Toolchain Issues
# These are issues either with the P4 compiler or the behavioral model executing the code.
# These issues needed to be tracked and fixed in P4C.
####################################################################################################
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Assertion"
  # Assertion 'Default switch case should not be reachable' failed,
  # file '../../include/bm/bm_sim/actions.h' line '369'.
  issue1607-bmv2.p4
  bmv2_copy_headers.p4
)
 
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "terminate called after throwing an instance"
  # terminate called after throwing an instance of 'std::out_of_range'
  # h.array[h.h.a].index
  # It turns out that h.h.a matters more than the size of the array
  bmv2_hs1.p4
  control-hs-index-test1.p4
  control-hs-index-test2.p4
  action_selector_shared-bmv2.p4

  # terminate called after throwing an instance of 'boost::wrapexcept<std::range_error>'
  issue2726-bmv2.p4
  runtime-index-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unsupported type argument for Value Set"
  pvs-nested-struct.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Invalid reference to object of type"
  extract_for_header_union.p4
)

####################################################################################################
# 2. P4Testgen Issues
# These are failures in P4Testgen that need to be fixed.
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Computations are not supported in update_checksum"
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "is trying to match on a tainted key set"
  # unimlemented feature (for select statement)
  invalid-hdr-warnings1.p4
  issue692-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Only registers with bit or int types are currently supported"
  issue907-bmv2.p4
)

####################################################################################################
# 3. WONTFIX
# These are failures that can not be solved by changing P4Testgen
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Assert error"
  # Assert/Assume error: assert/assume(false).
  bmv2_assert.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Assume error"
  # Assert/Assume error: assert/assume(false).
  bmv2_assume.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Computations are not supported in update_checksum"
  issue1765-bmv2.p4
  issue1765-1-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "error: exit"
  # exit: Conditional execution in actions unsupported on this target.
  issue2359.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "IfStatement"
  # IfStatement: not supported within a deparser on this target.
  issue887.p4
)
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Checksum16.get is deprecated and not supported."
  # Not supported
  issue841.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unknown or unimplemented extern method: increment"
  # user defined externs
  issue1882-1-bmv2.p4
  issue1882-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unknown or unimplemented extern method: update"
  issue2664-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unknown extern method count from type jnf_counter"
  # user defined extern
  issue1193-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unknown or unimplemented extern method: fn_foo"
  # user defined extern
  issue3091.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "BMv2 target only supports headers with fields totaling a multiple of 8 bits"
  custom-type-restricted-fields.p4
  issue3225.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "with type Type_Specialized is not a Type_Declaration"
  # Pipeline as a parameter of a switch, not a valid v1model program
  issue1304.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Program is not supported by this target"
  issue986-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
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
  "testgen-p4c-bmv2-ptf"
  "error.NoError: unsupported exact key expression"
  issue1062-bmv2.p4
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
  issue584-1-bmv2.p4
)

####################################################################################################
# 4. PARAMETERS NEEDED
####################################################################################################
# These tests require additional input parameters to compile properly.

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "uninitialized: next field read"
  # error: parsedHdr.hstack.next uninitialized: next field read
  # next not implemented in p4c/backends/bmv2/common/expression.cpp line 367
  next-def-use.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "headers: the type should be a struct of headers, stacks, or unions"
  parser-unroll-test3.p4
  parser-unroll-test4.p4
  parser-unroll-test5.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "The validity bit of .* is tainted"
  control-hs-index-test3.p4
  control-hs-index-test5.p4
)

####################################################################################################
# 5. PTF tests
# These are failures in PTF test that need to be fixed.
####################################################################################################

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Address family not supported by protocol"
  pvs-struct-2-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Expected packet was not received on device"
  issue2314.p4
  issue281.p4
  bmv2_lookahead_2.p4
  xor_test.p4
  parser-unroll-issue3537-1.p4
  parser-unroll-issue3537.p4
  parser-unroll-t1-cond.p4
  parser-unroll-test2.p4
  header-stack-ops-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "At index"
  # At index 0: UNKNOWN, 'Error when adding match entry to target'
  issue1062-1-bmv2.p4
  v1model-p4runtime-most-types1.p4
  pins_fabric.p4
  pins_wbb.p4
  v1model-p4runtime-enumint-types1.p4

  # At index 0: INVALID_ARGUMENT, 'Bytestring provided does not fit within 0 bits'
  pins_middleblock.p4
  issue2283_1-bmv2.p4

  # At index 0: INVALID_ARGUMENT, '0 is not a valid session id'
  issue1642-bmv2.p4
  issue1653-bmv2.p4
  issue1653-complex-bmv2.p4
  issue1660-bmv2.p4
  issue562-bmv2.p4
)

p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Unexpected error in RPC handling"
  # Unexpected error in RPC handling
  issue3374.p4
  control-hs-index-test6.p4
  parser-unroll-test1.p4
)


p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "Error when importing p4info"
  v1model-digest-containing-ser-enum.p4
  v1model-digest-custom-type.p4
)

# Using a wildcard because the actual error is not shown. It is a packet mismatch.
p4tools_add_xfail_reason(
  "testgen-p4c-bmv2-ptf"
  "ATTENTION: SOME TESTS DID NOT PASS!!!"
  issue383-bmv2.p4
)
