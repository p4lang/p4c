/*
Test that the compiler warns when an unrecognized diagnostic name is used
with @noWarn annotation.
*/

// Valid diagnostic name - should not produce warning
@noWarn("unused")
const bit<8> valid_unused = 1;

// Invalid diagnostic name - should produce warning
@noWarn("index_size")
const bit<8> invalid_index_size = 2;

// Another invalid diagnostic name - should produce warning
@noWarn("not-a-real-diagnostic")
const bit<8> invalid_other = 3;
