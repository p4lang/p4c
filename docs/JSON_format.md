# BMv2 JSON input format

All bmv2 target switches take as input a JSON file, whose format is essentially
target independent. The format is very simple and several examples can be found
in this repository, including [here]
(../targets/simple_router/simple_router.json).

This documents attempt to describe the expected JSON schema and the constraints
on each attribute. There is some ongoing work to write a formal JSON schema as
per [this specification] (http://json-schema.org/).

## Current bmv2 JSON format version

The version described in this document is *2.9*.

The major version number will be increased by the compiler only when
backward-compatibility of the JSON format is broken. After a major version
number increase, consummers of the JSON file may need to be updated or they may
not be able to consume newer JSON files.

Starting with version 2.0, the version number is included in the JSON file
itself, under the key `__meta__` -> `version`.

Note that the bmv2 code currently does not perform any version check, even
though it is recommended for all consummers of the JSON.

## General information

Tentative support for signed fields (with a 2 complement representation) has
been added to bmv2, although they are not supported in P4 1.0 or by the [p4c-bm
compiler] (https://github.com/p4lang/p4c-bm). However, signed constants (in
expressions, or as primitive arguments) are always supported.
Arithmetic is done with infinite precision, but when a value is copied into a
field, it is truncated based on the field's bitwidth.

## Format

### The type value object

You will see many places where a JSON object with 2 attributes, `type` and
`value`, is expected (for example, action primitive arguments). This is the
convention we follow:
- if `type` is `field`, `value` is a JSON 2-tuple, where the first item is the
header instance name and the second is the field member name.
- if `type` is `hexstr`, `value` is a bitstring written in hexadecimal (big
endian order); it can be prefixed with a negative sign, for negative values.
- if `type` is `bool`, `value` is either `true` or `false`.
- if `type` is a named P4 type (`header`, `header_stack`, `calculation`,
`register_array`, `meter_array`, `counter_array`), `value` is a string
corresponding to the name of the designated object.
- if `type` is `string`, `value` is a sequence of characters.
- if `type` is `lookahead` (parser only), `value` is a JSON 2-tuple, where the
first item is the bit offset for the lookahead and the second item is the
bitwidth.
- if `type` is `register`, `value` is a JSON 2-tuple, where the first item is
the register array name and the second is an `expression` used to evaluate the
index.
- if `type` is `stack_field`, `value` is a JSON 2-tuple, where the first item is
the header stack name and the second is the field member name. This is used to
access a field in the last valid header instance in the stack.
- if `type` is `expression`, `value` is a JSON object with 3 attributes:
  - `op`: the operation performed (`+`, `-`, `*`, `<<`, `>>`, `==`, `!=`, `>`,
  `>=`, `<`, `<=`, `and`, `or`, `not`, `&`, `|`, `^`, `~`, `valid`)
  - `left`: the left side of the operation, or `null` if unary operation
  - `right`: the right side of the operation

For an expression, `left` and `right` will themselves be JSON objects, where the
`value` attribute can be one of `field`, `hexstr`, `header`, `expression`,
`bool`, `register`, `header_stack`, `stack_field`.

bmv2 also supports these recently-added operations:
  - data-to-bool conversion (`op` is `d2b`): unary operation which can be used
to explicitly convert a data value (i.e. a value that can be read from /
written to a field, or a value obtained from runtime action data, or a value
obtained from a JSON `hexstr`) to a boolean value. Note that implicit
conversions are not supported.
  - bool-to-data conversion (`op` is `b2d`): unary operation which can be used
to explicitly convert a boolean value to a data value. Note that implicit
conversions are not supported.
  - 2-complement modulo (`op` is `two_comp_mod`): given a source data value and
a width data value, produces a third data value which is the signed value of the
source given a 2-complement representation with that width. For example,
`two_comp_mod(257, 8) == 1` and `two_comp_mod(-129, 8) == 127`.
  - ternary operator (`op` is `?`): in addition to `left` and `right`, the JSON
object has a fourth attribute, `cond` (condition), which is itself an
expression. For example, in `(hA.f1 == 9) ? 3 : 4`, `cond` would be the JSON
representation of `(hA.f1 == 9)`, `left` would be the JSON representation of `3`
and `right` would be the JSON representation of `4`.
  - stack header access (`op` is `dereference_stack`): `left` is a
`header_stack` and `right` needs to evaluate to a valid index inside the stack;
the expression produces a `header`.
  - last valid index in a stack (`op` is `last_stack_index`): unary operation
which takes a `header_stack` and produces the index of the last valid `header`
in the stack as a data value.
  - size of a stack (`op` is `size_stack`): unary operation which takes a
`header_stack` and produces the size of the stack (i.e. number of valid headers
in the stack) as a data value.
  - access to the header field at a given offset (`op` is `access_field`):
`left` needs to evaluate to a `header` and `right` is a JSON integer
representing a valid field offset for that header; the expression returns the
header field at the given offset. The interest of this operation is that the
`header` needs not be known at compile time, it can be a stack member resolved
at runtime.

For field references, some special values are allowed. They are called "hidden
fields". For now, we only support one kind of hidden fields: `<header instance
name>.$valid$`. This field is a 1-bit field which encodes the validity of the
corresponding header. It is a read-only field. It can be used just like any
other field; in particular as part of a match-table key or in a control-flow
condition. In the long run, this field will completely replace the valid match
type and the `valid()` built-in (in expressions), but we are not there yet.


The attributes of the root JSON dictionary are:

### `__meta__`

It is a JSON object which includes some meta information about the JSON file. It
has the following attributes:
- `version`: a JSON array with exactly 2 integer entries, a major version number
and a minor version number.
- `compiler`: an optional string to help identify the P4 compiler which produced
the JSON.

### `header_types`

It is a JSON array of all the header types defined in the P4 program. Each array
item has the following attributes:
- `name`: fully qualified P4 name of the header type
- `id`: a unique integer (unique with respect to other header types)
- `fields`: a JSON array of JSON 2-tuples (field member name, field width in
bits). Note that the JSON 2-tuples can optionally be JSON 3-tuples if you want
to experiment with signed fields support. In this case, the third element of the
tuples is a boolean value, which is `true` iff the field is signed. A signed
field needs to have a bitwidth of at least 2! For variable length fields, the
field width is `"*"`. There can be at most one variable-length field in a header
type. When a variable-length field is present, the header type is required to
have the `length_exp` attribute.
- `length_exp`: this attribute is only present when the header type includes a
variable-length field, in which case this attribute must be an expression
resolving to an integral value, corresponding to the variable-length field's
bitwidth.
- `max_length`: this attribute needs only be present when the header type
includes a variable-length field, in which case this attribute is the maximum
width in bytes for the header.

### `headers`

It is a JSON array of all the header instances (including metadata) declared in
the P4 program. Each array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other header instances)
- `header_type`: the `name` of the header type for this header instance
- `metadata`: a boolean value, `true` iff the instance is a metadata instance

### `header_stacks`

It is a JSON array of all the header stacks declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other header stacks)
- `header_type`: the `name` of the header type for this stack
- `header_ids`: a JSON array of integers; each integer being the unique `id` of
a header instance included in the stack. These ids have to be in the correct
order: stack[0], stack[1], ...

### `errors`

It is a JSON array of all the errors declared in the P4 program (error
declarations were introduced in P4_16 and can be raised in parsers). Each array
item is itself an array with exactly 2 elements: the name of the error as it
appears in the P4 program and an integer value in the range `[0, 2**31 - 1)`. It
is up to the compiler to assign a *unique* integer value to each declared error;
if the error constant is used in the parser (e.g. in a `verify` statement) or in
a control, it is up to the compiler to consistently replace each error with its
assigned value when producing the bmv2 JSON. bmv2 supports the core library
errors, but it is up to the compiler to include them in this JSON array and
assign a value to them (however this is not strictly necessary if these core
error constants are not used anywhere in the program).

We recommend using 0 for the core library error `NoError`, but this is not
strictly necessary. Note that the value `2**31 - 1` is reserved and cannot be
assigned by the compiler.

### `enums`

Its is a JSON array of all the enums declared in the P4 program (enum
declarations were introduced in P4_16). Each array item has the following
attributes:
- `name`: name of the enum, as it appears in the p4 program
- `entries`: a JSON array of all the constants declared inside the enum. Each
array item is itself an array with exactly 2 elements: the name of the enum
constant as it appears in the P4 program and an integer value in the range `[0,
2**31 - 1)`. It is up to the compiler to assign a *unique* integer value (unique
with respect to the other constants in the enum) to each enum constant; if the
enum constant is used in an expression in the P4 program, it is up to the
compiler to consistently replace each reference with its assigned value when
producing the bmv2 JSON. This is very similar to how we handle [errors]
(#errors).

### `parsers`

It is a JSON array of all the parsers declared in the P4 program. Each array
item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other parsers)
- `init_state`: the name of the start state for the parser
- `parse_states`: a JSON array of JSON objects. Each of these objects stores the
information for a given parse state in the program, which is used by the current
parser. The attributes for these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  parse states in the JSON file, not just the parse states included in this
  parser object
  - `parser_ops`: a JSON array of the operations (set or extract) performed in
  this parse state, in the correct order. Each parser operation is represented
  by a JSON object, whose attributes are:
    - `op`: the type of operation, either `extract`, `extract_VL`, `set`,
    `verify` or `shift`
    - `parameters`: a JSON array of objects encoding the parameters to the
    parser operation. Each parameter object has 2 string attributes: `type` for
    the parameter type and `value` for its value. Depending on the type of
    operation, the constraints are different. A description of these constraints
    is included [later in this section] (#parser-operations).
  - `transition_key`: a JSON array (in the correct order) of objects which
  describe the different fields of the parse state transition key. Each object
  has 2 attributes, `type` and `value`, where `type` can be either
  `field`, `stack_field` (for a field of the last extracted instance in a
  stack) or `lookahead` (see [here] (#the-type-value-object)).
  - `transitions`: a JSON array of objects encoding each parse state
  transition. The different attributes for these objects are:
    - `type`: either `default` (for the default transition), `hexstr` (for a
    regular hexstring value-based transition) or `parse_vset` (for a parse
    value-set).
    - `value`: only relevant if the `type` attribute is `hexstr`, in which case
    it is the hexstring value for the transition, or `parse_vset`, in which case
    it is the name of the corresponding parse value-set. Set to `null` if `type`
    is `hexstr`.
    - `mask`: only relevant if the `type` attribute is `hexstr` or `parse_vset`
    (`null` if `type` is `default`). It can either be a hexstring (to be used as
    a mask and ANDed with the key and the value) or `null`. For a parse
    value-set, the mask will be ANDed with each value in the set when checking
    for a match.
    - `next_state`: the name of the next parse state, or `null` if this is the
    last state in the parser.

For values and masks, make sure that you use the correct format. They need to be
the concatenation (in the right order) of all byte padded fields (padded with
`0` bits). For example, if the transition key consists of a 12-bit field and a
2-bit field, each value will need to have 3 bytes (2 for the first field, 1 for
the second one). If the transition value is `0xaba`, `0x3`, the `value`
attribute will be set to `0x0aba03`.

#### parser operations

In the `parser_ops` array, the format of the `parameters` array depends on the
`op` value:
  - `extract`: only takes one parameter, of type `regular` (extraction to a
  regular header instance) or `stack` (extraction to the end of a header
  stack). `value` is then the name of the header instance or stack.
  - `extract_VL`: introduced for P4_16, where the expression to dynamically
  compute the length of a variable-length field is an argument to the extract
  built-in rather than a property of the header. For this operation, we require
  2 parameters, the first one of type `regular` and the second one of type
  `expression` (to compute the length in bits of the variable-length field in
  the header).
  - `set`: takes exactly 2 parameters; the first one needs to be of type `field`
  with the appropriate value. The second one can be of type `field`, `hexstr`,
  `lookahead` or `expression`, with the appropriate value (see [here]
  (#the-type-value-object)).
  - `verify`: we expect an array with exactly 2 elements; the first should be a
  boolean expression while the second should be an expression resolving to a
  valid integral value for an error constant (see [here] (#errors)).
  - `shift`: we expect a single parameter, the number of bytes to shift (shifted
  packet data will be discarded).

### `parse_vsets`

It is a JSON array of all the parse value-sets declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other parse value-sets)
- `compressed_bitwidth`: the bitwidth of the values which can be added to the
set. Note that this bitwidth does not include any padding.

### `deparsers`

It is a JSON array of all the deparsers declared in the P4 program (or inferred
from P4 parsers). Each array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other deparsers)
- `order`: a JSON array of sorted header instance names. When the target switch
invokes a deparser, the headers will be serialized in this order, and non-valid
headers will be skipped.

### `meter_arrays`

It is a JSON array of all the meter arrays declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other meter arrays)
- `type`: either `bytes` or `packets`
- `rate_count`: the number of rates for the meters. By default P4 uses 2-rate
3-color meters
- `size`: the number of meter instances in the array
- `is_direct`: a boolean indicating whether this is a direct meter array
- `binding`: if the meter array is direct, the name of the table it is attached
to
- `result_target`: a 2-tuple representing the field where the meter result
(color) will be stored; only taken into account if `is_direct` is `true`.

### `counter_arrays`

It is a JSON array of all the counter arrays declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other counter arrays)
- `size`: the number of counter instances in the array
- `is_direct`: a boolean indicating whether this is a direct counter array
- `binding`: if the counter array is direct, the name of the table it is
attached to

Unlike for meter arrays, there is no `type` attribute because bmv2 counters
always count both bytes and packets.

### `register_arrays`

It is a JSON array of all the register arrays declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other register arrays)
- `size`: the number of register instances in the array
- `bitwidth`: the width in bits of each register cell

### `actions`

It is a JSON array of all the actions declared in the P4 program. Each array
item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other actions)
- `runtime_data`: a JSON array of objects, each one representing a named
parameter of the action. Each object has exactly 2 attributes:
  - `name`: the P4 name of the parameter
  - `bitwidth`: the integral width, in bits, of the parameter
- `primitives`: a JSON array of objects, each one representing a primitive
call, with the following attributes:
  - `op`: the primitive name. This primitive has to be supported by the target.
  - `parameters`: a JSON array of the arguments passed to the primitive (has to
  match the target primitive definition). Each argument is represented by a JSON
  object with the following attributes:
    - `type`: one of `hexstr`, `runtime_data`, `header`, `field`, `calculation`,
    `meter_array`, `counter_array`, `register_array`, `header_stack`,
    `expression`, `extern`, `string`, 'stack_field'
    - `value`: the appropriate parameter value. If `type` is `runtime_data`,
    this is an integer representing an index into the `runtime_data` (attribute
    of action) array. If `type` is `extern`, this is the name of the extern
    instance. See [here] (#the-type-value-object) for other types.

*Important note about extern instance methods*: even though in P4 these are
invoked using object-oriented style, bmv2 treats them as regular primitives for
which the first parameter is the extern instance. For example, if the P4 call is
`extern1.methodA(x, y)`, where `extern1` is an extern instance of type
`my_extern_type`, the bmv2 compiler needs to translate this into a call to
primitive `_my_extern_type_methodA`, with the first parameter being `{"type":
"extern", "value": "extern1"}` and the second parameter being the appropriate
representation for `x` and `y`.

bmv2 supports three core primitives: `assign`, `assign_VL` (for variable-length
fields) and `assign_header`. Support for additional primitives depends on the
architecture being used.

### `pipelines`

It is a JSON array of all the top control flows (ingress, egress) in the P4
program. Each array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other pipelines)
- `init_table`: the name of the first table of the pipeline
- `action_profiles`: a JSON array of JSON objects. Each of these objects stores
the information for a given P4 action profile, which is used by the current
pipeline. The attributes for these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  action profiles in the JSON file, not just the action profiles included in
  this pipeline object
  - `max_size`: an integer representing the size (number of entries) of the
  action profile
  - `selector`: only present if the action profile supports dynamic group member
  selection. It needs to be a JSON object with these attributes:
    - `algo`: the hash algorithm used for group member selection (has to be
    supported by target switch)
    - `input`: a JSON array of objects with the following attributes:
      - `type`: has to be `field`
      - `value`: the field reference
- `tables`: a JSON array of JSON objects. Each of these objects stores the
information for a given P4 table, which is used by the current pipeline. The
attributes for these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  tables in the JSON file, not just the tables included in this pipeline object
  - `match_type`: one of `exact`, `lpm`, `ternary` or `range`
  - `type`: the implementation for the table, one of `simple`, `indirect`
  (action profiles), `indirect_ws` (action profiles with dynamic selector)
  - `action_profile`: if the table is indirect, name of the action profile to
  use with this table. If the table type is `indirect_ws`, then the referenced
  action profile needs to have a selector attribute.
  - `max_size`: an integer representing the size (number of entries) of the
  table
  - `with_counters`: a boolean, `true` iff the match table has direct counters
  - `support_timeout`: a boolean, `true` iff the match table supports ageing
  - `key`: the lookup key format, represented by a JSON array. Each member of
  the array is a JSON object with the following attributes:
    - `match_type`: one of `valid`, `exact`, `lpm`, `ternary`, `range`
    - `target`: the field reference as a 2-tuple (or header as a string if
      `match_type` if `valid`)
    - `mask`: the static mask to be applied to the field, or null. Just like for
    the parser transition key, make sure that this mask is byte-padded and has
    the same width (in bytes) as the corresponding field (1 byte if `match_type`
    is `valid`).
  - `actions`: the list of actions (order does not matter) supported by this
  table
  - `next_tables`: maps each action to a next table name. Alternatively, maps
  special string `__HIT__` and `__MISS__` to a next table name.
  - `direct_meters`: the name of the associated direct meter array, or null if
  the match table has no associated meter array
  - `default_entry`: an optional JSON item which can force the default entry for
  the table to be set when loading the JSON, without intervention from the
  control plane. It has the following attributes:
    - `action_id`: the id of the default action
    - `action_const`: an optional boolean value which is `true` iff the control
    plane is not allowed to change the default action function. Default value is
    `false`.
    - `action_data`: an optional JSON array where each entry is the hexstring
    value for an action argument. The size of the array needs to match the
    number of parameters expected by the action function with id `action_id`.
    - `action_entry_const`: an optional boolean value which is `true` iff the
    control plane is not allowed to modify the action entry (action function +
    action data). Default value is `false`. This attribute is ignored if the
    `action_data` attribute it missing.
  - `entries`: enables you to optionally specify match-action entries for this
  table. Specifying entries in the JSON makes the table immutable, which means
  the added entries cannot be modified / deleted and that new entries cannot be
  added. This doesn't impact the default entry though (see the `default_entry`
  attribute). `entries` is a JSON array where each element follows the
  [match-action entry format] (#match-action-entry-format) described below.
- `conditionals`: a JSON array of JSON objects. Each of these objects stores the
information for a given P4 condition, which is used by the current pipeline. The
attributes for these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  conditions in the JSON file, not just the conditions included in this parser
  object
  - `expression`: the expression for the condition. See [here]
    (#the-type-value-object) for more information on expressions format.

The `match_type` for the table needs to follow the following rules:
- If one match field is `range`, the table `match_type` has to be `range`
- If one match field is `ternary`, the table `match_type` has to be `ternary`
- If one match field is `lpm`, the table `match_type` is either `ternary` or
`lpm`
Note that it is not correct to have more than one `lpm` match field in the same
table.

#### match-action entry format

We describe the format of the match-action entries contained in the `entries`
JSON array (see table JSON format above). Each entry is a JSON object with the
following attributes:
- `match_key`: a JSON array of objects with the following attributes:
  - `match_type`: one of `valid`, `exact`, `lpm`, `ternary`, `range`
  - the other attributes depend on the `match_type`:
    - for `valid`: we need a `key` attribute which has to be a boolean
    - for `exact`: we need a `key` attribute which has to be a hexstring
    - for `lpm`: we need a `key` attribute which has to be a hexstring and a
    `prefix_length` attribute which has to be an integer
    - for `ternary`: we need a `key` and a `mask` attributes which both have to
    be hexstrings
    - for `range`: we need a `start` and a `end` attribute which both have to be
    hexstrings
- `action_entry`: a JSON object with the following attributes:
  - `action_id`: the id of the default action
  - `action_data`: a JSON array where each entry is the hexstring value for an
  action argument. The size of the array needs to match the number of parameters
  expected by the action function with id `action_id`.
- `priority`: an integer which indicates the priority for the entry; it is
ignored if the match type of the table is not `ternary` or `range`. In bmv2
having a high priority translates into a low numerical value.

Note that the `match_key` and the `action_entry` need to be formatted
correctly. In particular:
- the number of match fields in the match key needs to match the table
description
- the match types of the match fields in the match key need to match the table
description
- the hexstrings need to have the correct byte-width
- duplicate entries (as determined by the match keys) will trigger an error

### `calculations`

It is a JSON array of all the named calculations in the P4 program. In
particular, they are used for checksums. Each array item has the following
attributes:
- `name`
- `id`: a unique integer (unique with respect to other calculations)
- `algo`: the hash algorithm used (has to be supported by target switch)
- `input`: a JSON array of objects with the following attributes:
  - `type`: one of `field`, `hexstr`, `header`, `payload`
  - `value`: the appropriate value or reference (see [here]
    (#the-type-value-object))

If `type` is `payload`, all the headers present after the last included header
(or after the enclosing header of the last included field) will be included in
the input, as well as the packet payload. This is necessary for TCP checksum
computation.

If a header is not valid when the calculation is evaluated, it will be skipped.

### `checksums`

It is a JSON array of all the checksums in the P4 program. Each array item has
the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other checksums)
- `target`: the field where the checksum result is written
- `type`: always set to `generic`
- `calculation`: the name of the calculation to use to compute the checksum
- `if_cond`: null if the checksum needs to be updated unconditionally, otherwise
a boolean expression, which will determine whether or not the checksum gets
updated. See [here]
    (#the-type-value-object) for more information on expressions format.

### `learn_lists`

Not documented yet, use empty JSON array.

### `extern_instances`

It is a JSON array of all the extern instances in the P4 program. Each array
item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other extern instances)
- `type`: the name of the extern type, the target switch needs to support this
type
- `attribute_values`: a JSON array with the initial values for the attributes of
this extern instance. Each array item has the following attributes:
  - `name`: the name of the attribute
  - `type`: the type of the attribute, only `hexstr` (integral values), `string`
  (for character sequences) and `expression` are supported for now
  - `value`: the initial value for the attribute
