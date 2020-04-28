# BMv2 JSON input format

All bmv2 target switches take as input a JSON file, whose format is essentially
target independent. The format is very simple and several examples can be found
in this repository, including
[here](../targets/simple_router/simple_router.json).

This documents attempt to describe the expected JSON schema and the constraints
on each attribute.

## Current bmv2 JSON format version

The version described in this document is *2.23*.

The major version number will be increased by the compiler only when
backward-compatibility of the JSON format is broken. After a major version
number increase, consummers of the JSON file may need to be updated or they may
not be able to consume newer JSON files.

Starting with version 2.0, the version number is included in the JSON file
itself, under the key `__meta__` -> `version`.

Note that the bmv2 code will perform a version check against the provided input
JSON.

## General information

Tentative support for signed fields (with a 2 complement representation) has
been added to bmv2, although they are not supported in P4 1.0 or by the [p4c-bm
compiler](https://github.com/p4lang/p4c-bm). However, signed constants (in
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
`register_array`, `meter_array`, `counter_array`, `header_union`,
`header_union_stack`), `value` is a string corresponding to the name of the
designated object.
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
- if `type` is `union_stack_field`, `value` is a JSON 3-tuple, where the first item is
the header union stack name, the second is the union member name and the third
is the field member name. This can be used exclusively in the `transition_key`
of a parser to access a field in the last valid union instance in the stack.
- if `type` is `expression`, `value` is a JSON object with 3 attributes:
  - `op`: the operation performed (`+`, `-`, `*`, `<<`, `>>`, `==`, `!=`, `>`,
  `>=`, `<`, `<=`, `and`, `or`, `not`, `&`, `|`, `^`, `~`, `valid`,
  `valid_union`)
  - `left`: the left side of the operation, or `null` if unary operation
  - `right`: the right side of the operation
- if `type` is `local`, `value` is an integer representing an index inside an
array of local integral values. This `type` can only be used inside of an
expression, i.e. if this JSON object has a parent whose `type` attribute is
`expression`. The meaning of these "local integral values" depend on the context
in which the parent expression is evaluated. In the case of an expression being
evaluated inside of an action function belonging to a match-action table, the
local integral values correspond to the runtime data available for a given table
entry (see the [actions](#actions) section for more details on runtime data).
- if `type` is `parameters_vector`, `value` is a JSON array containing an
arbitrary number of "type value objects".

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
  - saturating cast for signed integers (`op` is `sat_cast`).
  - saturating cast for unsigned integers (`op` is `usat_cast`).
  - ternary operator (`op` is `?`): in addition to `left` and `right`, the JSON
object has a fourth attribute, `cond` (condition), which is itself an
expression. For example, in `(hA.f1 == 9) ? 3 : 4`, `cond` would be the JSON
representation of `(hA.f1 == 9)`, `left` would be the JSON representation of `3`
and `right` would be the JSON representation of `4`.
  - stack header access (`op` is `dereference_header_stack`): `left` is a
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
  - stack union access (`op` is `dereference_union_stack`): `left` is a
`header_union_stack` and `right` needs to evaluate to a valid index inside the
stack; the expression produces a `header`.
  - access to the header union member at a given offset (`op` is
`access_union_header`): `left` needs to evaluate to a `header_union` and `right`
is a JSON integer representing a valid member offset for that union; the
expression returns the header at the given offset. The interest of this
operation is that the `header_union` needs not be known at compile time, it can
be a union stack entry resolved at runtime.

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

### `header_union_types`

It is a JSON array of all the header union types declared in the P4
program. Each array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other header union types)
- `headers`: a JSON array of 2-tuples, where the first element is the P4 name of
the corresponding union member, and the second element is the name of the header
type for this element.

### `header_unions`

It is a JSON array of all the header unions declared in the P4 program. Each
array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other header unions)
- `union_type`: the name of the corresponding header union type for this union
instance
- `header_ids`: a JSON array of integers, each integer being the unique `id` of
a header instance included in the union. We recommend using the same order as in
the corresponding P4 declaration.

### `header_union_stacks`

It is a JSON array of all the header union stacks declared in the P4
program. Each array item has the following attributes:
- `name`
- `id`: a unique integer (unique with respect to other header union stacks)
- `union_type`: the name of the corresponding header union type for the elements
of this stack.
- `header_union_ids`: a JSON array of integers, each integer being the unique
`id` of a header union instance included in the stack. These ids have to be in
the correct order: union_stack[0], union_stack[1], ...

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
producing the bmv2 JSON. This is very similar to how we handle
[errors](#errors).

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
  - `parser_ops`: a JSON array of the operations performed in this parse state,
  in the correct order. Each parser operation is represented by a JSON object,
  whose attributes are:
    - `op`: the type of operation, either `extract`, `extract_VL`, `set`,
    `verify`, `shift`, `advance` or `primitive`.
    - `parameters`: a JSON array of objects encoding the parameters to the
    parser operation. Depending on the type of operation, the constraints are
    different. A description of these constraints is included [later in this
    section](#parser-operations).
  - `transition_key`: a JSON array (in the correct order) of objects which
  describe the different fields of the parse state transition key. Each object
  has 2 attributes, `type` and `value`, where `type` can be either
  `field`, `stack_field` (for a field of the last extracted instance in a
  stack), `union_stack_field` (for a field of the last extracted instance in a
  union stack) or `lookahead` (see [here](#the-type-value-object)).
  - `transitions`: a JSON array of objects encoding each parse state
  transition. The different attributes for these objects are:
    - `type`: either `default` (for the default transition), `hexstr` (for a
    regular hexstring value-based transition) or `parse_vset` (for a parse
    value-set).
    - `value`: only relevant if the `type` attribute is `hexstr`, in which case
    it is the hexstring value for the transition, or `parse_vset`, in which case
    it is the name of the corresponding parse value-set. Set to `null` if `type`
    is `default`.
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
  regular header instance), `stack` (extraction to the end of a header stack) or
  `union_stack` (extraction to the end of a header union stack). If `type` is
  `regular`, `value` is the name of the header instance to extract. If `type` is
  `stack`, `value` is the name of the header stack. Finally if `type` is
  `union_stack`, then `value` is a 2-tuple with the name of the header union
  stack as the first element and the name of the appropriate union member as the
  second element.
  - `extract_VL`: introduced for P4_16, where the expression to dynamically
  compute the length of a variable-length field is an argument to the extract
  built-in rather than a property of the header. For this operation, we require
  2 parameters. The first one follows the same rules as `extract`'s first and
  only parameter. The second one must be of type `expression` (to compute the
  length in bits of the variable-length field in the header). bmv2 currently
  requires this expression to evaluate to a multiple of 8.
  - `set`: takes exactly 2 parameters; the first one needs to be of type `field`
  with the appropriate value. The second one can be of type `field`, `hexstr`,
  `lookahead` or `expression`, with the appropriate value (see
  [here](#the-type-value-object)).
  - `verify`: we expect an array with exactly 2 elements; the first should be a
  boolean expression while the second should be an expression resolving to a
  valid integral value for an error constant (see [here](#errors)).
  - `shift`: we expect a single parameter, the number of bytes to shift (shifted
  packet data will be discarded). `shift` is deprecated in favor of `advance`
  (see below).
  - `advance`: we expect a single parameter, the number of *bits* to shift,
  which can be of type `hexstr`, `field` or `expression`. `advance` replaces
  `shift`. bmv2 currently requires the number of bits to shift to be a multiple
  of 8.
  - `primitive`: introduced for P4_16, where extern methods can be called from
  parser states. It only takes one parameter, which is itself a JSON object with
  the following attributes:
    - `op`: the primitive name. This primitive has to be supported by the
      target.
    - `parameters`: a JSON array of the arguments passed to the primitive (has
  to match the target primitive definition). Each argument is represented by a
  [type-value](#the-type-value-object) JSON object. This is consistent with how
  control-flow actions are described in the JSON, so you can refer to the
  [actions](#actions) section for more details.

We provide an example of parser operations for a parser state:
```json
"parser_ops": [
    {
        "op": "extract",
        "parameters": [
            {"type": "regular", "value": "ethernet"}
        ]
    },
    {
        "op": "primitive",
        "parameters": [
            {
                "op": "assign_header",
                "parameters": [
                    {"type": "header", "value": "ethernet_copy"},
                    {"type": "header", "value": "ethernet"},
                ]
            }
        ]
    }
]
```

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

For stacks and unions, all the header instances need to be listed in the
appropriate order.

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
    instance. See [here](#the-type-value-object) for other types.

An `expression` can either correspond to an lvalue expression or an rvalue
expression. For example, the P4 expression `hdr.h2[hdr.h1.idx].v = hdr.h1.v + 7`
corresponds to the following JSON object (which would be an entry in the
`primitives` JSON array for the action to which the expression belongs):
```json
{
    "op" : "assign",
    "parameters" : [
        {
            "type" : "expression",
            "value": {
                "type": "expression",
                "value": {
                    "op": "access_field",
                    "left": {
                        "type": "expression",
                        "value": {
                            "op": "dereference_header_stack",
                            "left": {
                                "type": "header_stack",
                                "value": "h2"
                            },
                            "right": {
                                "type": "field",
                                "value": ["h1", "idx"]
                            }
                        }
                    },
                    "right": 0
                }
            }
        },
        {
            "type" : "expression",
            "value": {
                "type": "expression",
                "value": {
                    "op": "+",
                    "left": {
                        "type" : "field",
                        "value" : ["h1", "v"]
                    },
                    "right": {
                        "type" : "hexstr",
                        "value" : "0x0000004d"
                    }
                }
            }
        }
    ]
}
```
In this case, the left-hand side of the assignment (`hdr.h2[hdr.h1.idx].v`) is
an lvalue, while the right-hand side is an rvalue (`hdr.h1.v + 77`). However,
this information does not need to appear *explicitly* in the JSON. The bmv2
implementation of the `assign` primitive requires the first parameter to be a
lvalue expression, and we assume that the compiler will not generate a JSON that
violates this requirement. An invalid JSON will lead to undefined behavior.

*Important note about extern instance methods*: even though in P4 these are
invoked using object-oriented style, bmv2 treats them as regular primitives for
which the first parameter is the extern instance. For example, if the P4 call is
`extern1.methodA(x, y)`, where `extern1` is an extern instance of type
`my_extern_type`, the bmv2 compiler needs to translate this into a call to
primitive `_my_extern_type_methodA`, with the first parameter being `{"type":
"extern", "value": "extern1"}` and the second parameter being the appropriate
representation for `x` and `y`.

bmv2 supports the following core primitives:
- `assign`, `assign_VL` (for variable-length fields), `assign_header`,
`assign_union`, `assign_header_stack` and `assign_union_stack`.
- `push` and `pop` for stack (header stack or header union stack) manipulation.
- `_jump`: takes one parameter which must resolve to an integral value
`offset`. When it is executed, we jump to the primitive call at index `offset`
in the enclosing action.
- `_jump_if_zero`: implements a conditional jump and takes two parameters which
must both resolve to integral values, which we call respectively `cond` and
`offset`. If `cond` is 0, the primitive behaves exactly like `_jump` and we jump
to the primitive call at index `offset`; otherwise the primitive is a no-op.
- `exit`: implements the P4 built-in `exit` statement. The packet will be
"marked for exit" by setting a dedicated flag; it is currently the
responsibility of the target to reset the flag between pipelines if so desired.
- `assume` and `assert`: these 2 statements are used in P4 for formal program
verification. They both take a single parameter (a boolean expression) and they
share the same implementation in bmv2 (the switch will log an error message and
abort), but they mean different things: *assume* statements are essentially
inputs to the formal verification tool, while *assert* statements are conditions
that need to be proven true (see
[this](https://github.com/p4lang/p4c/issues/1548) Github issue for more
details). Given their bmv2 implementation, `assert` and `assume` statements are
also useful when tetsing / debugging P4 programs with bmv2.
- `log_msg`: used for logging user-defined messages that will be output to the
console when using the `--log-console` option. It takes two parameters: a format
string (which may contain one or more curly braces '{}') and a vector of
parameters (`parameters_vector`), which is a list of values to substitute to the
curly braces included in the format string when outputting the log message. See
[here](#the-type-value-object) for more information on `parameters_vector`. The
objects in the `parameters_vector` must resolve to integral values.

Support for additional primitives depends on the architecture being used.

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
    - `name`: this attribute is optional. If present it must be a string value
    which corresponds to the name of the element being matched on. This was
    introduced because in P4_16 match key elements can be arbitary
    expressions and even though bmv2 only supports matching on fields (or
    headers), we need to have a way to preserve the original name (generated by
    the compiler or provided by the programmer with the `@name` annotation). As
    for bmv2 itself, this information is only used in log messages. If the
    attribute is omitted, bmv2 will generate a default name from the `target`
    attribute.
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
    `false`. It can only be set to `true` for `simple` tables.
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
  [match-action entry format](#match-action-entry-format) described below.
- `conditionals`: a JSON array of JSON objects. Each of these objects stores the
information for a given P4 condition, which is used by the current pipeline. The
attributes for these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  conditions in the JSON file, not just the conditions included in this pipeline
  object
  - `expression`: the expression for the condition. See
    [here](#the-type-value-object) for more information on expressions format.
  - `true_next`: the name of the next control flow to execute if the condition
  evaluates to true (can be a table, another conditional or an action call), or
  null if this is the end of the pipeline
  - `false_next`: the name of the next control flow to execute if the condition
  evaluates to false, or null if this is the end of the pipeline
- `action_calls`: a JSON array of JSON objects. It is used for direct action
calls from a control flow which are not wrapped into a table. The attributes for
these objects are:
  - `name`
  - `id`: a unique integer; note that it has to be unique with respect to *all*
  action calls in the JSON file, not just the ones included in this pipeline
  object
  - `action_id`: the id of the action to call; note that the corresponding
  action must not expect any parameter
  - `next_node`: the name of the next control flow node to execute (can be a
  table, a conditional or another action call like this one), or null if this is
  the last node in the pipeline

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
  - `value`: the appropriate value or reference (see
    [here](#the-type-value-object)). For `hexstr`, we require an extra attribute
    (in addition to `type` and `value`), `bitwidth`, whose value must be a
    positive integer equal to the desired bitwidth for the constant. Note that
    we require this attribute because the bitwidth cannot be inferred from the
    hexadecimal string directly, as the desired bitwidth may not be a multiple
    of a byte.

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
- `update`: an optional boolean value, which defaults to `true`; indicates
whether the checksum needs to be updated in deparsers.
- `verify`: an optional boolean value, which defaults to `true`; indicates
whether the checksum needs to be verified in parsers.
- `if_cond`: null if the checksum needs to be verified and / or updated
unconditionally, otherwise a boolean expression, which will determine whether or
not the checksum gets verified and / or updated. See
[here](#the-type-value-object) for more information on expressions format.

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

### `field_aliases`

bmv2 target architectures usually require a set of metadata fields to be defined
in the JSON input. For example, simple_switch requires the following fields:
`standard_metadata.ingress_port`, `standard_metadata.packet_length`,
`standard_metadata.instance_type`, `standard_metadata.egress_spec`, and
`standard_metadata.egress_port`. These fields
happen to be the standard metadata fields described in the P4_14
specification. In some cases you may want to use different names for these
fields in a P4 program, which you can accomplish by using field aliases. A field
alias maps the name expected by bmv2 to the name used in the P4 / JSON.

The `field_aliases` attribute is a JSON array of 2-tuples, with the following
elements:
- a string corresponding to the name expected by the bmv2 target
(e.g. `"standard_metadata.egress_port"`)
- a 2-tuple where the first item is the actual header instance name and the
second is the actual field member name, as they appear in the P4 / JSON input
(e.g. `["my_metadata", "my_egress_port"]`)
