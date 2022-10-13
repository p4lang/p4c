# XFAILS: tests that *temporarily* fail
# ================================================
# Xfails are _temporary_ failures: the tests should work but we haven't fixed p4testgen yet.

# ###################################################################################################
# 1. P4C Toolchain Issues
# ###################################################################################################
# These are issues either with the P4 compiler or the behavioral model executing the code.
# These issues needed to be tracked and fixed in P4C.

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "simple_switch died with return code -6"

    # Assertion 'Default switch case should not be reachable' failed,
    # file '../../include/bm/bm_sim/actions.h' line '369'.
    issue1607-bmv2.p4 # passed after adding of --loopsUnroll flag
    bmv2_copy_headers.p4 # passed after adding of --loopsUnroll flag

    # terminate called after throwing an instance of 'std::out_of_range'
    # h.array[h.h.a].index
    # It turns out that h.h.a matters more than the size of the array
    bmv2_hs1.p4 # passed after adding HsIndexSimplifier to Simple Switch
    control-hs-index-test1.p4 # passed after adding HsIndexSimplifier to Simple Switch
    control-hs-index-test2.p4 # passed after adding HsIndexSimplifier to Simple Switch

    # terminate called after throwing an instance of 'boost::wrapexcept<std::range_error>'
    # Conversion from negative integer to an unsigned type results in undefined behaviour
    issue2726-bmv2.p4 # passed after adding HsIndexSimplifier to Simple Switch
    runtime-index-bmv2.p4 # passed after adding HsIndexSimplifier to Simple Switch
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "terminate called after throwing an instance of"

    # terminate called after throwing an instance of 'std::runtime_error'
    # in Json::Value::operator[](ArrayIndex)const: requires arrayValue
    control-hs-index-test6.p4 # passed after adding HsIndexSimplifier to Simple Switch
    issue3374.p4

    # terminate called after throwing an instance of 'std::runtime_error'
    # Type is not convertible to string
    control-hs-index-test3.p4 # passed after adding HsIndexSimplifier to Simple Switch
    parser-unroll-test1.p4 # passed after adding of --loopsUnroll flag
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "bad json"

    # crashed after adding of --loopsUnroll flag
    # bug after last HsIndexSimplifier (was bad json)
    invalid-hdr-warnings4.p4
    control-hs-index-test5.p4 # If statement is not supported for this target after HSIndexSimplifier
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "differs|Expected ([0-9]+) packets on port ([0-9]+) got ([0-9]+)"
      # This test fails because we do not have the priority annotation implemented.
    issue281.p4 # bug in parserunroll
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Exception"
    # Running simple_switch_CLI: Exception  Unexpected key field &
    match-on-exprs2-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Invalid reference to object of type"
    extract_for_header_union.p4
)

# ###################################################################################################
# 2. P4Testgen Issues
# ###################################################################################################
# These are failures in P4Testgen that need to be fixed.
p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown or unimplemented extern method: recirculate_preserving_field_list"
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown or unimplemented extern method: extract"
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Non-numeric, non-boolean member expression"
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Computations are not supported in update_checksum"
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "with type .* is not a Constant"
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Cast failed"

    # push front can not handled tainted header validity.
    header-stack-ops-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "is trying to match on a tainted key set"
    invalid-hdr-warnings1.p4 # unimlemented feature (for select statement)
    issue692-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Only registers with bit or int types are currently supported"
    issue907-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Could not find type for"

    # Related to postorder(IR::Member* expression) in ParserUnroll,
    # and more specifically when member is passed to getTypeArray
)

# ###################################################################################################
# 3. WONTFIX
# ###################################################################################################
# These are failures that can not be solved by changing P4Testgen
p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Assert/assume can not be executed under a tainted condition"

    # Assert/Assume error: assert/assume(false).
    bmv2_assert.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "simple_switch died with return code -6"

    # Assert/Assume error: assert/assume(false).
    bmv2_assume.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Computations are not supported in update_checksum"
    issue1765-1-bmv2.p4
    issue1765-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "error: exit"
    issue2359.p4 # exit: Conditional execution in actions unsupported on this target.
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Error compiling"

    # IfStatement: not supported within a deparser on this target.
    issue887.p4
)
p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Checksum16.get is deprecated and not supported."
    issue841.p4 # Not supported
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown or unimplemented extern method: increment"
    issue1882-1-bmv2.p4 # user defined extern
    issue1882-bmv2.p4 # user defined extern
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown or unimplemented extern method: update"
    issue2664-bmv2.p4 # user defined extern
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown extern method count from type jnf_counter"
    issue1193-bmv2.p4 # user defined extern
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Unknown or unimplemented extern method: fn_foo"
    issue3091.p4 # user defined extern
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "The BMV2 architecture requires 6 pipes"

    # All of these tests are not valid bmv2 programs but import v1model.
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
    "testgen-p4c-bmv2-emi"
    "BMv2 target only supports headers with fields totaling a multiple of 8 bits"
    custom-type-restricted-fields.p4
    issue3225.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "with type Type_Specialized is not a Type_Declaration"

    # Pipeline as a parameter of a switch, not a valid v1model program
    issue1304.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Program is not supported by this target"
    issue986-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "Program can not be implemented on this target"

    # The error appears because the table calls itself:
    # apply {
    # simple_table.apply();
    # simple_table.apply();
    # }
    # if you remove one of the calls, then the test will pass, but at the end there will be an error:
    # Transmitting packet of size 1333 out of port 0
    issue2344.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "error.NoError: unsupported exact key expression"
    issue1062-bmv2.p4
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "expected type to be a struct"
    issue3394.p4
)

# ###################################################################################################
# 4. PARAMETERS NEEDED
# ###################################################################################################
# These tests require additional input parameters to compile properly.
p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "uninitialized: next field read"

    # error: parsedHdr.hstack.next uninitialized: next field read
    # next not implemented in p4c/backends/bmv2/common/expression.cpp line 367
    next-def-use.p4 # passed after adding of --loopsUnroll flag
)

p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
    "headers: the type should be a struct of headers, stacks, or unions"
    parser-unroll-test3.p4
    parser-unroll-test5.p4
    parser-unroll-test4.p4
)

# TODO: For these test we should add the --permissive flag.
p4tools_add_xfail_reason(
    "testgen-p4c-bmv2-emi"
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