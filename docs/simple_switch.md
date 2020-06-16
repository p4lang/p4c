# The BMv2 Simple Switch target

The bmv2 framework lets developers implement their own P4-programmable
architecture as a software switch. The simple_switch architecture is the
de-facto architecture for most users, as it is roughly equivalent to the
"abstract switch model" described in the [P4_14 spec](https://p4.org/specs).

The P4_16 language has been designed such that there can be multiple
architectures, e.g. one or more architectures for a switch device, one
or more architectures for a NIC device, etc.  The v1model architecture
is included with the p4c compiler, and was designed to be identical to
the P4_14 switch architecture, enabling straightforward
auto-translation from P4_14 programs to P4_16 programs that use the
v1model architecture.  There are a few tiny differences between P4_14
and v1model called out below, primarily in the names of a few metadata
fields.

The P4_16 language also now has a Portable Switch Architecture (PSA)
defined in [its own specification](https://p4.org/specs).  As of
October 2019, a partial implementation of the PSA architecture has
been done, but it is not yet complete.  It will be implemented in a
separate executable program named `psa_switch`, separate from the
`simple_switch` program described here.

This document aims at providing P4 programmers with important information
regarding the simple_switch architecture.

## Standard metadata

For a P4_16 program using the v1model architecture and including the
file `v1model.p4`, all of the fields below are part of the struct with
type `standard_metadata_t`.

For a P4_14 program, all of the fields below are part of the header
named `standard_metadata`, which is predefined for you.

The following abbreviations are used to mark field names below, to
indicate where they appear:

- sm14 - the field is defined in v1.0.4 of the P4_14 language
  specification, Section 6 titled "Standard Intrinsic Metadata".
- v1m - the field is defined in the include file `p4include/v1model.p4`
  of the [p4c](https://github.com/p4lang/p4c) repository, intended
  to be included in P4_16 programs compiled for the v1model
  architecture.

Here are the fields:

- `ingress_port` (sm14, v1m) - For new packets, the number of the
  ingress port on which the packet arrived to the device.  Read only.
- `packet_length` (sm14, v1m) - For new packets from a port, or
  recirculated packets, the length of the packet in bytes.  For cloned
  or resubmitted packets, you may need to include this in a list of
  fields to preserve, otherwise its value will become 0.
- `egress_spec` (sm14, v1m) - Can be assigned a value in ingress code to
  control which output port a packet will go to.  The P4_14 primitive
  `drop`, and the v1model primitive action `mark_to_drop`, have the side
  effect of assigning an implementation specific value DROP_PORT to this field
  (511 decimal for simple_switch by default, but can be changed through
  the `--drop-port` target-specific command-line option), such that if
  `egress_spec` has that value at the end of ingress processing, the
  packet will be dropped and not stored in the packet buffer, nor sent
  to egress processing.  See the "after-ingress pseudocode" for relative
  priority of this vs. other possible packet operations at end of
  ingress.  If your P4 program assigns a value of DROP_PORT to `egress_spec`, it
  will still behave according to the "after-ingress pseudocode", even if you
  never call `mark_to_drop` (P4_16) or `drop` (P4_14).
- `egress_port` (sm14, v1m) - Only intended to be accessed during
  egress processing, read only.  The output port this packet is
  destined to.
- `egress_instance` (sm14) - Renamed `egress_rid` in simple_switch.
  See `egress_rid` below.
- `instance_type` (sm14, v1m) - Contains a value that can be read by
  your P4 code.  In ingress code, the value can be used to distinguish
  whether the packet is newly arrived from a port (`NORMAL`), it was
  the result of a resubmit primitive action (`RESUBMIT`), or it was the
  result of a recirculate primitive action (`RECIRC`).  In egress processing,
  can be used to determine whether the packet was produced as the
  result of an ingress-to-egress clone primitive action (`INGRESS_CLONE`),
  egress-to-egress clone primitive action (`EGRESS_CLONE`), multicast
  replication specified during ingress processing (`REPLICATION`), or
  none of those, so a normal unicast packet from ingress (`NORMAL`).
  Until such time as similar constants are pre-defined for you, you
  may copy [this
  list](https://github.com/p4lang/p4c/blob/master/testdata/p4_14_samples/switch_20160512/includes/intrinsic.p4#L62-L68)
  of constants into your code.
- `parser_status` (sm14) or `parser_error` (v1m) - `parser_status` is
  the name in the P4_14 language specification.  It has been renamed
  to `parser_error` in v1model.  The value 0 (sm14) or error.NoError
  (P4_16 + v1model) means no error.  Otherwise, the value indicates
  what error occurred during parsing.
- `parser_error_location` (sm14) - Not present in v1model.p4, and not
  implemented in simple_switch.
- `checksum_error` (v1m) - Read only. 1 if a call to the
  `verify_checksum` primitive action finds a checksum error, otherwise
  0.  Calls to `verify_checksum` should be in the `VerifyChecksum`
  control in v1model, which is executed after the parser and before
  ingress.

## Intrinsic metadata

Each architecture usually defines its own intrinsic metadata fields, which are
used in addition to the standard metadata fields to offer more advanced
features. In the case of simple_switch, we have two separate intrinsic metadata
headers. These headers are not strictly required by the architecture as it is
possible to write a P4 program and run it through simple_switch without them
being defined. However, their presence is required to enable some features of
simple_switch. For most of these fields, there is no strict requirement as to
the bitwidth, but we recommend that you follow our suggestions below. Some of
these intrinsic metadata fields can be accessed (read and / or write) directly,
others should only be accessed through primitive actions.

### `intrinsic_metadata` header

For a P4_16 program using the v1model architecture and including the
file `v1model.p4`, all of the fields below are part of the struct with
type `standard_metadata_t`.  There is no need to define your own
struct type for these fields.

For a P4_14 program, we recommend that you define and instantiate this
metadata header for every P4 program that you write for the
simple_switch architecture, with the following code:
```
header_type intrinsic_metadata_t {
    fields {
        ingress_global_timestamp : 48;
        egress_global_timestamp : 48;
        mcast_grp : 16;
        egress_rid : 16;
    }
}
metadata intrinsic_metadata_t intrinsic_metadata;
```
- `ingress_global_timestamp`: a timestamp, in microseconds, set when the packet
shows up on ingress. The clock is set to 0 every time the switch starts. This
field can be read directly from either pipeline (ingress and egress) but should
not be written to.  See Section [BMv2 timestamp implementation notes](#BMv2-timestamp-implementation-notes) for more details.
- `egress_global_timestamp`: a timestamp, in microseconds, set when the packet
starts egress processing. The clock is the same as for
`ingress_global_timestamp`. This field should only be read from the egress
pipeline, but should not be written to.
- `mcast_grp`: needed for the multicast feature. This field needs to be written
in the ingress pipeline when you wish the packet to be multicast. A value of 0
means no multicast. This value must be one of a valid multicast group configured
through bmv2 runtime interfaces. See the "after-ingress pseudocode" for
relative priority of this vs. other possible packet operations at
end of ingress.
- `egress_rid`: needed for the multicast feature. This field is only valid in
the egress pipeline and can only be read from. It is used to uniquely identify
multicast copies of the same ingress packet.

### `queueing_metadata` header

For a P4_16 program using the v1model architecture and including the
file `v1model.p4`, all of the fields below are part of the struct with
type `standard_metadata_t`.  There is no need to define your own
struct type for these fields.

For a P4_14 program,
you only need to define this P4 header if you want to access queueing
information - as a reminder, the packet is queued between the ingress and
egress pipelines. Note that this header is "all or nothing". Either you do not
define it or you define it with all its fields (there are 4 of them). We
recommend that you use the following P4_14 code:
```
header_type queueing_metadata_t {
    fields {
        enq_timestamp : 48;
        enq_qdepth : 16;
        deq_timedelta : 32;
        deq_qdepth : 16;
        qid : 8;
    }
}
metadata queueing_metadata_t queueing_metadata;
```
Of course, all of these fields can only be accessed from the egress pipeline and
they are read-only.
- `enq_timestamp`: a timestamp, in microseconds, set when the packet is first
enqueued.
- `enq_qdepth`: the depth of the queue when the packet was first enqueued, in units of number of packets (not the total size of packets).
- `deq_timedelta`: the time, in microseconds, that the packet spent in the
queue.
- `deq_qdepth`: the depth of queue when the packet was dequeued, in units of number of packets (not the total size of packets).
- `qid`: when there are multiple queues servicing each egress port (e.g. when
priority queueing is enabled), each queue is assigned a fixed unique id, which
is written to this field. Otherwise, this field is set to 0.
TBD: `qid` is not currently part of type `standard_metadata_t` in v1model.
Perhaps it should be added?

## Supported primitive actions

We mostly support the standard P4_14 primitive actions. One difference is that
optional parameters are not supported in bmv2, so all parameters are always
required (see `resubmit` for example).
The full list of primitives can be seen in this [C++ source
file](../targets/simple_switch/primitives.cpp).

## Pseudocode for what happens at the end of ingress and egress processing

After-ingress pseudocode - the short version:

```
if (a clone primitive action was called) {
    create clone(s) of the packet with details configured for the clone session
}
if (digest to generate) {   // because your code called generate_digest
    send a digest message to the control plane software
}
if (resubmit was called) {
    start ingress processing over again for the original packet
} else if (mcast_grp != 0) {  // because your code assigned a value to mcast_grp
    multicast the packet to the output port(s) configured for group mcast_grp
} else if (egress_spec == DROP_PORT) {  // e.g. because your code called drop/mark_to_drop
    Drop packet.
} else {
    unicast the packet to the port equal to egress_spec
}
```

After-ingress pseudocode - for determining what happens to a packet
after ingress processing is complete.  The longer more detailed
version:

```
if (a clone primitive action was called) {
    // This condition will be true if your code called the `clone` or
    // `clone3` primitive action from a P4_16 program, or the
    // `clone_ingress_pkt_to_egress` primitive action in a P4_14
    // program, during ingress processing.

    Create zero or more clones of the packet.  The cloned packet(s)
    will be enqueued in the packet buffer, destined for the egress
    port(s) configured in the clone session whose numeric id was given
    as the value of the `session` parameter when the last clone
    primitive action was called.

    Each cloned packet will later perform egress processing,
    independently from whatever the original packet does next, and if
    multiple cloned packets are created via the same clone operation,
    independently from each other.

    The contents of the cloned packet(s) will be the same as the
    packet when it most recently began ingress processing, where the
    clone operation was performed, without any modifications that may
    have been made during the execution of that ingress code.  (That
    may not be the packet as originally received by the switch, if the
    packet reached this occurrence of ingress processing via a
    recirculate operation, for example.)

    If it was a clone3 (P4_16) or clone_ingress_pkt_to_egress (P4_14)
    action, also preserve the final ingress values of the metadata
    fields specified in the field list argument, except assign
    instance_type a value of PKT_INSTANCE_TYPE_INGRESS_CLONE.

    Each cloned packet will be processed by your parser code again.
    In many cases this will result in exactly the same headers being
    parsed as when the packet was most recently parsed, but if your
    parser code uses the value of standard_metadata.instance_type to
    affect its behavior, it could be different.

    After this parsing is done, each clone will continue processing at
    the beginning of your egress code.
    // fall through to code below
}
if (digest to generate) {
    // This condition will be true if your code called the
    // generate_digest primitive action during ingress processing.
    Send a digest message to the control plane that contains the
    values of the fields in the specified field list.
    // fall through to code below
}
if (resubmit was called) {
    // This condition will be true if your code called the resubmit
    // primitive action during ingress processing.
    Start ingress over again for this packet, with its original
    unmodified packet contents and metadata values.  Preserve the
    final ingress values of any fields specified in the field list
    given as an argument to the last resubmit() primitive operation
    called, except assign instance_type a value of PKT_INSTANCE_TYPE_RESUBMIT.
} else if (mcast_grp != 0) {
    // This condition will be true if your code made an assignment to
    // standard_metadata.mcast_grp during ingress processing.  There
    // are no special primitive actions built in to simple_switch for
    // you to call to do this -- use a normal P4_16 assignment
    // statement, or P4_14 modify_field() primitive action.
    Make 0 or more copies of the packet based upon the list of
    (egress_port, egress_rid) values configured by the control plane
    for the mcast_grp value.  Enqueue each one in the appropriate
    packet buffer queue.  The instance_type of each will be
    PKT_INSTANCE_TYPE_REPLICATION.
} else if (egress_spec == DROP_PORT) {
    // This condition will be true if your code called the
    // mark_to_drop (P4_16) or drop (P4_14) primitive action during
    // ingress processing.
    Drop packet.
} else {
    Enqueue one copy of the packet destined for egress_port equal to
    egress_spec.
}
```

After-egress pseudocode - the short version:

```
if (a clone primitive action was called) {
    create clone(s) of the packet with details configured for the clone session
}
if (egress_spec == DROP_PORT) {  // e.g. because your code called drop/mark_to_drop
    Drop packet.
} else if (recirculate was called) {
    after deparsing, deparsed packet starts over again as input to parser
} else {
    Send the packet to the port in egress_port.
}
```

After-egress pseudocode - for determining what happens to a packet
after egress processing is complete.  The longer more detailed
version:

```
if (a clone primitive action was called) {
    // This condition will be true if your code called the `clone` or
    // `clone3` primitive action from a P4_16 program, or the
    // `clone_egress_pkt_to_egress` primitive action in a P4_14
    // program, during egress processing.

    Create zero or more clones of the packet.  The cloned packet(s)
    will be enqueued in the packet buffer, destined for the egress
    port(s) configured in the clone session whose numeric id was given
    as the value of the `session` parameter when the last clone
    primitive action was called.

    Each cloned packet will later perform egress processing,
    independently from whatever the original packet does next, and if
    multiple cloned packets are created via the same clone operation,
    independently from each other.

    The contents of the cloned packet(s) will be as they are at the
    end of egress processing, including any changes made to the values
    of fields in headers, and whether headers were made valid or
    invalid.  Your deparser code will _not_ be executed for
    egress-to-egress cloned packets, nor will your parser code be
    executed for them.

    If it was a clone3 (P4_16) or clone_egress_pkt_to_egress (P4_14)
    action, also preserve the final egress values of the metadata
    fields specified in the field list argument, except assign
    instance_type a value of PKT_INSTANCE_TYPE_EGRESS_CLONE.  Each
    cloned packet will have the same standard_metadata fields
    overwritten that all packets that begin egress processing do,
    e.g. egress_port, egress_spec, egress_global_timestamp, etc.

    Each cloned packet will continue processing at the beginning of
    your egress code.
    // fall through to code below
}
if (egress_spec == DROP_PORT) {
    // This condition will be true if your code called the
    // mark_to_drop (P4_16) or drop (P4_14) primitive action during
    // egress processing.
    Drop packet.
} else if (recirculate was called) {
    // This condition will be true if your code called the recirculate
    // primitive action during egress processing.
    Start processing the packet over again.  That is, take the packet
    as constructed by the deparser, with any modifications made to the
    packet during both ingress and egress processing, and give that
    packet as input to the parser.  Preserve the final egress values
    of any fields specified in the field list given as an argument to
    the last recirculate primitive action called, except assign
    instance_type a value of PKT_INSTANCE_TYPE_RECIRC.
} else {
    Send the packet to the port in egress_port.  Since egress_port is
    read only during egress processing, note that its value must have
    been determined during ingress processing for normal packets.  One
    exception is that a clone primitive action executed during egress
    processing will have its egress_port determined from the port that
    the control plane configured for that clone session).
}
```


## Table match kinds supported

`simple_switch` supports table key fields with any of the following `match_kind`
values:

+ `exact` - from P4_16 language specification
+ `lpm` - from P4_16 language specification
+ `ternary` - from P4_16 language specification
+ `optional` - defined in `v1model.p4`
+ `range` - defined in `v1model.p4`
+ `selector` - defined in `v1model.p4`

`selector` is only supported for tables with an action selector
implementation.

If a table has more than one `lpm` key field, it is rejected by the `p4c` BMv2
back end. This could be generalized slightly, as described below, but that
restriction is in place as of the January 2019 version of `p4c`.


### Range tables

If a table has at least one `range` field, it is implemented internally as a
`range` table in BMv2. Because a single search key could match multiple entries,
every entry must be assigned a numeric priority by the control plane software
when it is installed. If multiple installed table entries match the same search
key, one among them with the maximum numeric priority will "win", and its action
performed.

Note that winner is one with maximum numeric priority value if you use the
P4Runtime API to specify the numeric priorities. Check the documentation of your
control plane API if you use a different one, as some might choose to use the
convention that minimum numeric priority values win over larger ones.

A `range` table may have an `lpm` field. If so, the prefix length is used to
determine whether a search key matches the entry, but the prefix length does
_not_ determine the relative priority among multiple matching table
entries. Only the numeric priority supplied by the control plane software
determines that. Because of this, it would be reasonable for a `range` table to
support multiple `lpm` key fields, but as of January 2020 this is not supported.

If a range table has entries defined via a `const entries` table property, then
the relative priority of the entries are highest priority first, to lowest
priority last, based upon the order they appear in the P4 program.


### Ternary tables

If a table has no `range` field, but at least one `ternary` or `optional` field,
it is implemented internally as a `ternary` table in BMv2. As for `range`
tables, a single search key can be matched by multiple table entries, and thus
every entry must have a numeric priority assigned by the control plane
software. The same note about `lpm` fields described above for `range` tables
also applied to `ternary` tables, as well as the note about entries specified
via `const entries`.


### Longest prefix match tables

If a table has no `range`, `ternary`, nor `optional` fields, but at least one
`lpm` field, there must be exactly one `lpm` field. There can be 0 or more
`exact` fields. While there can be multiple installed table entries that match a
single search key, with these restrictions there can be at most one matching
table entry of each possible prefix length of the `lpm` field (because no two
table entries installed at the same time are allowed to have the same search
key). The matching entry with the longest prefix length is always the
winner. The control plane cannot specify a priority when installing entries for
such a table -- it is always determined by the prefix length.

If a longest prefix match table has entries defined via a `const entries` table
property, then the relative priority of the entries are determined by the prefix
lengths, not by the order they appear in the P4 program.


### Exact match tables

If a table has only `exact` fields, it is implemented internally as an `exact`
table in BMv2. For any search key, there can be at most one matching table
entry, because duplicate search keys are not allowed to be installed. Thus no
numeric priority is ever needed to determine the "winning" matching table entry.
BMv2 (and many other P4 implementations) implements the match portion of such a
table's functionality using a hash table.

If an exact match table has entries defined via a `const entries` table
property, there can be at most one matching entry for any search key, so the
relative order that entries appear in the P4 program is unimportant in
determining which entry will win.


## Specifying match criteria for table entries using `const entries`

The table below shows, for each `match_kind` value of P4_16 table key fields,
which syntax is allowed for specifying the set of matching field values for a
table entry in a `const entries` list.

A restriction on all allowed cases is that `lo`, `hi`, `val`, and `mask` must be
a legal possible value for the field, i.e. not outside of that field's range of
values. All of them can be arithmetic expressions containing compile time
constant values.

|                  | `range`      | `ternary`    | `optional`   | `lpm`        | `exact` |
| ---------------- | ------------ | ------------ | ------------ | ------------ | ------- |
| `lo .. hi`       | yes (Note 1) | no           | no           | no           | no      |
| `val &&& mask`   | no           | yes (Note 2) | no           | yes (Note 3) | no      |
| `val`            | yes (Note 4) | yes (Note 5) | yes          | yes (Note 5) | yes     |
| `_` or `default` | yes (Note 6) | yes (Note 6) | yes (Note 6) | yes (Note 6) | no      |

Note 1: Restriction: `lo <= hi`. A run time search key value `k` matches if `lo
<= k <= hi`.

Note 2: Restriction: `val == (val & mask)`. Bit positions of `mask` equal to 1
are exact match bit positions, and bit positions where `mask` is 0 are "wild
card" or don't care bit position. A run time search key value `k` matches if `(k
& mask) == (val & mask)`.

Note 3: Restriction: `val == (val & mask)` and `mask` is a "prefix mask",
i.e. it has all 1 bit positions consecutive and in the most significant bit
positions of the field. WARNING: If you attempt to specify a prefix as `val /
prefix_length` in a P4_16 program (the syntax used by some command line
interfaces for specifying a prefix, such as `simple_switch_CLI`), that is
actually an arithmetic expression that divides `val` by `prefix_length`, and
thus falls into the `val` case, which is exact match. There is no warning from
the compiler because it is perfectly legal syntax for a division operation.

Note 4: Equivalent to the range `val .. val`, so it behaves as exact match on
`val`.

Note 5: Equivalent to `val &&& mask` where `mask` is 1 in all bit positions of
the field, so it behaves as exact match on `val`.

Note 6: Matches any allowed value for the field. Equivalent to
`min_posible_field_value .. max_possible_field_value` for `range` fields, or `0
&&& 0` for `ternary` and `lpm` fields.

Below is a portion of a P4_16 program that demonstrates most of the allowed
combinations of `match_kind` and syntax for specifying matching sets of values.

```
header h1_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    h1_t h1;
}

// ... later ...

control ingress(inout headers_t hdr,
                inout metadata_t m,
                inout standard_metadata_t stdmeta)
{
    action a(bit<9> x) { stdmeta.egress_spec = x; }
    table t1 {
        key = { hdr.h1.f1 : range; }
        actions = { a; }
        const entries = {
             1 ..  8 : a(1);
             6 .. 12 : a(2);  // ranges are allowed to overlap between entries
            15 .. 15 : a(3);
            17       : a(4);  // equivalent to 17 .. 17
            // It is not required to have a "match anything" rule in a table,
            // but it is allowed (except for exact match fields), and several of
            // these examples have one.
            _        : a(5);
        }
    }
    table t2 {
        key = { hdr.h1.f1 : ternary; }
        actions = { a; }
        // There is no requirement to specify ternary match criteria using
        // hexadecimal values.  I personally prefer it to make the mask bit
        // positions more obvious.
        const entries = {
            0x04 &&& 0xfc : a(1);
            0x40 &&& 0x72 : a(2);
            0x50 &&& 0xff : a(3);
            0xfe          : a(4);  // equivalent to 0xfe &&& 0xff
            _             : a(5);
        }
    }
    table t3 {
        key = {
            hdr.h1.f1 : optional;
            hdr.h1.f2 : optional;
        }
        actions = { a; }
        const entries = {
            // Note that when there are two or more fields in the key of a
            // table, const entries key select expressions must be surrounded by
            // parentheses.
            (47, 72) : a(1);
            ( _, 72) : a(2);
            ( _, 75) : a(3);
            (49,  _) : a(4);
            _        : a(5);
        }
    }
    table t4 {
        key = { hdr.h1.f1 : lpm; }
        actions = { a; }
        const entries = {
            0x04 &&& 0xfc : a(1);
            0x40 &&& 0xf8 : a(2);
            0x04 &&& 0xff : a(3);
            0xf9          : a(4);  // equivalent to 0xf9 &&& 0xff
            _             : a(5);
        }
    }
    table t5 {
        key = { hdr.h1.f1 : exact; }
        actions = { a; }
        const entries = {
            0x04 : a(1);
            0x40 : a(2);
            0x05 : a(3);
            0xf9 : a(4);
        }
    }
    // ... more code here ...
}
```


## Restrictions on recirculate, resubmit, and clone operations

Recirculate, resubmit, and clone operations _that do not preserve
metadata_, i.e. they have an empty list of fields whose value should
be preserved, work as expected when using p4c and simple_switch, with
either P4_14 as the source language, or P4_16 plus the v1model
architecture.

Unfortunately, at least some of these operations that attempt to
preserve metadata _do not work correctly_ -- they still cause the
packet to be recirculated, resubmitted, or cloned, but they do not
preserve the desired metadata field values.

This is a known issue that has been discussed at length by the p4c
developers and P4 language design work group, and the decision made
is:

* Long term, the P4_16 [Portable Switch
  Architecture](https://p4.org/specs/) uses a different mechanism for
  specifying metadata to preserve than the v1model architecture uses,
  and should work correctly.  As of October 2019 the implementation of
  PSA is not complete, so this does not help one write working code
  today.
* If someone wishes to volunteer to make changes to p4c and/or
  simple_switch for improving this situation, please contact the P4
  language design work group and coordinate with them.
  * The preferred form of help would be to complete the Portable
    Switch Architecture implementation.
  * Another possibility would be enhancing the v1model architecture
    implementation.  Several approaches for doing so that have been
    discussed are described
    [here](https://github.com/jafingerhut/p4-guide/blob/master/v1model-special-ops/README-resubmit-examples.md).

Note that this issue affects not only P4_16 programs using the v1model
architecture, but also P4_14 programs compiled using the `p4c`
compiler, because internally `p4c` translates P4_14 to P4_16 code
before the part of the compiler that causes the problem to occur.

The fundamental issue is that P4_14 has the `field_list` construct,
which is a restricted kind of _named reference_ to other fields.  When
these field lists are used in P4_14 for recirculate, resubmit, and
clone operations that preserve metadata, they direct the target not
only to read those field values, but also later write them (for the
packet after recirculating, resubmitting, or cloning).  P4_16 field
lists using the syntax `{field_1, field_2, ...}` are restricted to
accessing the current values of the fields at the time the statement
or expression is executed, but do not represent any kind of reference,
and by the P4_16 language specification cannot cause the values of
those fields to change at another part of the program.


### Hints on distinguishing when metadata is preserved correctly

Below are some details for anyone trying, by hook or by crook, to make
preservation of metadata work with the current p4c implementation,
without the future improvements mentioned above.

There is no known simple rule to determine which invocations of these
operations will preserve metadata correctly and which will not.  If
you want at least some indication, examine the BMv2 JSON file produced
as output from the compiler.

The Python program
[bmv2-json-check.py](https://github.com/p4pktgen/p4pktgen/blob/master/tools/bmv2-json-check.py)
attempts to determine whether any field list used to preserve metadata
fields for recirculate, resubmit, or clone operations looks like it
has the name of a compiler-generated temporary variable.  Warning: its
methods are fairly basic, and thus there is no guarantee that if the
script says there is no problem, metadata preservation will work
correctly, or conversely that if the script says there are suspicious
things found, metadata preservation will not work correctly.  It
automates what is described below.

Every recirculate, resubmit, and clone operation is represented in
JSON data inside the BMv2 JSON file.  Search for one of these strings,
_with_ the double quote marks around them:

* `"recirculate"` - only parameter is a field_list id
* `"resubmit"` - only parameter is a field_list id
* `"clone_ingress_pkt_to_egress"` - second parameter is a field_list id
* `"clone_egress_pkt_to_egress"` - second parameter is a field_list id

You can find the fields in each field list in the section of the BMv2
JSON file with the key `"field_lists"`.  Whatever the field names are
there, simple_switch _will preserve the values of fields with those
names_.  The primary issue is whether those fields correspond to the
same storage locations that the compiler uses to represent the fields
you want to preserve.

In some cases they are, which is often the case if the field names in
the BMv2 JSON file look the same as, or similar to, the field names in
your P4 source code.

In some cases they represent different storage locations,
e.g. compiler-generated temporary variables used to hold a copy of the
field you want to preserve.  A field name beginning with `tmp.` is a
hint of this as of p4c 2019-Oct, but this is a p4c implementation
detail that could change.


## P4_16 plus v1model architecture notes

This section is an incomplete description of the v1model architecture
for P4_16.  Much of the earlier parts of this document are also a
description of the P4_16 v1model architecture, but in some cases they
also serve to document the P4_14 behavior of `simple_switch`.  This
section is only for things specific to P4_16 plus the v1model
architecture.

In some cases, the details documented here are for how the v1model
architecture is implemented in BMv2 simple_switch.  Such details are
prefaced with the "BMv2 v1model implementation".  There may be other
implementations of the v1model architecture that make different
implementation choices or restrictions.


### Restrictions on type `H`

There is a type called `H` that is a parameter to the package
`V1Switch` definition in `v1model.p4`, excerpted below:

```P4
package V1Switch<H, M>(Parser<H, M> p,
                       VerifyChecksum<H, M> vr,
                       Ingress<H, M> ig,
                       Egress<H, M> eg,
                       ComputeChecksum<H, M> ck,
                       Deparser<H> dep
                       );
```

This type `H` is restricted to be a struct that contains member fields
of one of the following types, and no others:

+ header
+ header_union
+ header stack


### Restrictions on parser code

The P4_16 language specification version 1.1 does not support `if`
statements inside of parsers.  Because the `p4c` compiler in-lines
calls to functions at the location of the call, this restriction
extends to function bodies of functions called from a parser.  There
are two potential workarounds:

+ If the conditional execution can be done just as effectively by
  waiting to do it during ingress control processing, you can use `if`
  statements there.
+ In some cases you can implement the desired conditional execution
  effect using `transition select` statements to branch to different
  parser states, each of which can have its own distinct code.

In the `v1model` architecture, a packet reaching the `reject` parser
state, e.g. because it failed a `verify` call, is _not_ automatically
dropped.  Such a packet will begin `Ingress` processing with the value
of the `parser_error` standard metadata field equal to the error that
occurred.  Your P4 code can direct such packets to be dropped if you
wish, but you may also choose to write code that will do other things
with such packets, e.g. send a clone of them to a control CPU for
further analysis or error logging.

`p4c` plus `simple_switch` does not support transitioning to the
`reject` state explicitly in the source code.  It can only do so via
failing a call to `verify`.


### Restrictions on code in the `VerifyChecksum` control

The `VerifyChecksum` control is executed just after the `Parser`
completes, and just before the `Ingress` control begins.  `p4c` plus
`simple_switch` only support a sequence of calls to the
`verify_checksum` or `verify_checksum_with_payload` extern functions
inside such controls.  See the `v1model.p4` include file for their
definition.  The first argument to these functions is a boolean, which
can be an arbitrary boolean condition used to make the checksum
calculation conditional on that expression being true.

In the `v1model` architecture, a packet having an incorrect checksum
is _not_ automatically dropped.  Such a packet will begin `Ingress`
processing with the value of the `checksum_error` standard metadata
field equal to 1.  If your program has multiple calls to
`verify_checksum` and/or `verify_checksum_with_payload`, there is no
way supported to determine which of the calls was the one that found
an incorrect checksum.  Your P4 code can direct such packets to be
dropped if you wish, but this will not automatically be done for you.


### Restrictions on code in the `Ingress` and `Egress` controls

`simple_switch` does not support doing `apply` on the same table more
than once for a single execution of the `Ingress` control, nor for a
single execution of the `Egress` control.

In some cases, you can work around this restriction by having multiple
tables with similar definitions, and `apply` each of them once per
execution of the `Ingress` control (or `Egress`).  If you want two
such tables to contain the same set of table entries and actions, then
you must write your control plane software to keep their contents the
same, e.g. always add an entry to `T2` every time you add an entry to
`T1`, etc.

This restriction is one that could be generalized in `simple_switch`,
but realize that some high speed ASIC implementations of P4 may also
impose this same restriction, because the restriction can be imposed
by the design of the hardware.


### Restrictions on code within actions

These restrictions are actually restrictions of the `p4c` compiler, not of
`simple_switch`.  Anyone interested in enhancing `p4c` to remove these
restrictions should see the issues below.

The P4_16 language specification v1.1.0 permits `if` statements within action
declarations.  `p4c`, when compiling for the target BMv2 simple_switch, supports
some kinds of `if` statements, in particular ones that can be transformed into
assignments using the ternary `condition ? true_expr : false_expr` operator.
This is supported:

```
    action foo() {
        meta.b = meta.b + 5;
        if (hdr.ethernet.etherType == 7) {
            hdr.ethernet.dstAddr = 1;
        } else {
            hdr.ethernet.dstAddr = 2;
        }
    }
```

but this is not, as of 2019-Jul-01:

```
    action foo() {
        meta.b = meta.b + 5;
        if (hdr.ethernet.etherType == 7) {
            hdr.ethernet.dstAddr = 1;
        } else {
            mark_to_drop(standard_metadata);
        }
    }
```

Given the following text from the P4_16 language specification, it is likely
that there are other P4 implementations that have limited or no support for `if`
statements within actions:

    No `switch` statements are allowed within an action --- the grammar permits
    them, but a semantic check should reject them.  Some targets may impose
    additional restrictions on action bodies --- e.g., only allowing
    straight-line code, with no conditional statements or expressions.

Thus P4 programs using `if` statements within actions are likely to be less
portable than programs that avoid doing so.

As mentioned above, enhancing `p4c` would enable a larger variety of `if`
statements within actions to be supported.

* [p4c issue #644](https://github.com/p4lang/p4c/issues/644)
* [behavioral-model issue #379](https://github.com/p4lang/behavioral-model/pull/379)


### Restrictions on code in the `ComputeChecksum` control

The `ComputeChecksum` control is executed just after the `Egress`
control completes, and just before the `Deparser` control begins.
`p4c` plus `simple_switch` only support a sequence of calls to the
`update_checksum` or `update_checksum_with_payload` extern functions
inside such controls.  The first argument to these functions is a
boolean, which can be an arbitrary boolean condition used to make the
checksum update action conditional on that expression being true.


### Restrictions on code in the `Deparser` control

The `Deparser` control is restricted to contain only a sequence of
calls to the `emit` method of the `packet_out` object.

The most straightforward approach to avoiding the restrictions in the
`ComputeChecksum` and `Deparser` controls is to write the more general
code you want near the end of the `Egress` control.


### BMv2 `register` implementation notes

Both p4c and simple_switch support register arrays with elements that
are arbitrary values of type `bit<W>` or `int<W>`, but not other
types, e.g. structs would be convenient for this purpose in some
programs, but this is not supported.

As an example of a way to work around this limitation, suppose you
wanted a struct with three fields x, y, and z with types `bit<8>`,
`bit<3>`, and `bit<6>`.  You can emulate this by making a register
array with elements of type `bit<17>` (the total width of all 3
fields), and use P4_16 bit slicing operations to separate the 3 fields
from the 17-bit value after reading from the register array, and P4_16
bit vector concatenation operations to combine 3 fields together,
forming a 17-bit result, before writing the 17-bit value to the
register array.

```
    register< bit<17> >(512) my_register_array;
    bit<9> index;

    // ... other code here ...

    // This example action is written assuming that some code executed
    // earlier (not shown here) has assigned a correct desired value
    // to the variable 'index'.

    action update_fields() {
        bit<17> tmp;
        bit<8> x;
        bit<3> y;
        bit<6> z;

        my_register_array.read(tmp, (bit<32>) index);
        // Use bit slicing to extract out the logically separate parts
        // of the 17-bit register entry value.
        x = tmp[16:9];
        y = tmp[8:6];
        z = tmp[5:0];

        // Whatever modifications you wish to make to x, y, and z go
        // here.  This is just an example code snippet, not known to
        // do anything useful for packet processing.
        if (y == 0) {
            x = x + 1;
            y = 7;
            z = z ^ hdr.ethernet.etherType[3:0];
        } else {
            // leave x unchanged
            y = y - 1;
            z = z << 1;
        }

        // Concatenate the updated values of x, y, and z back into a
        // 17-bit value for writing back to the register.
        tmp = x ++ y ++ z;
        my_register_array.write((bit<32>) index, tmp);
    }
```

While p4c and simple_switch support as wide a bit width W as you wish
for array elements for packet processing, the Thrift API (used by
`simple_switch_CLI`, and perhaps some switch controller software) only
supports control plane read and write operations for array elements up
to 64 bits wide (see the type `BmRegisterValue` in file
[`standard.thrift`](https://github.com/p4lang/behavioral-model/blob/master/thrift_src/standard.thrift),
which is a 64-bit integer as of October 2019).  The P4Runtime API does
not have this limitation, but there is no P4Runtime implementation of
register read and write operations yet for simple_switch:
[p4lang/PI#376](https://github.com/p4lang/PI/issues/376)

The BMv2 v1model implementation supports parallel execution.  It uses
locking of all register objects accessed within an action to guarantee
that the execution of all steps within an action are atomic, relative
to other packets executing the same action, or any action that
accesses some of the same register objects.  You need not use the
`@atomic` annotation in your P4_16 program in order for this level of
atomicity to be guaranteed for you.

BMv2 v1model as of October 2019 _ignores_ `@atomic` annotations in
your P4_16 programs.  Thus even if you use such annotations, this does
not cause BMv2 to treat any block of code larger than one action call
as an atomic transaction.


### BMv2 `random` implementation notes

The BMv2 v1model implementation of the `random` function supports the
`lo` and `hi` parameters being run-time variables, i.e. they need not
be compile time constants.

Also, they need not be limited to the constraint that `(hi - lo + 1)`
is a power of 2.

Type `T` is restricted to be `bit<W>` for `W <= 64`.


### BMv2 `hash` implementation notes

The BMv2 v1model implementation of the `hash` function supports `base`
and `max` parameters being run-time variables, i.e. the need not be
compile time constants.

Also, `max` need not be limited to the constraint that it is a power
of 2.

Call the hash value that is calculated from the data H.  The value
written to the out parameter named `result` is: `(base + (H % max))`
if `max >= 1`, otherwise `base`.

The type `O` of the `result`, `T` of `base`, and `M` of `max` are
restricted to be `bit<W>` for `W <= 64`.


### BMv2 `direct_counter` implementation notes

If a table `t` has a `direct_counter` object `c` associated with it by
having a table property `counters = c` in its definition, the BMv2
v1model implementation behaves as if every action for that table
contains exactly one call to `c.count()`, whether it has none, one, or
more than one.


### BMv2 `direct_meter` implementation notes

If a table `t` has a `direct_meter` object `m` associated with it by
having a table property `meters = m` in its definition, then _at least
one_ of that table's actions must have a call to
`m.read(result_field);`, for some field `result_field` with type
`bit<W>` where `W >= 2`.

When this is done, the BMv2 v1model implementation behaves as if _all_
actions for table `t` have such a call in them, even if they do not.

It is not supported, and the `p4c` compiler gives an error message, if
you have two actions for table `t` where one has the action
`m.read(result_field1)` and another has the action
`m.read(result_field2)`, or if both of those calls are in the same
action.  All calls to the `read()` method for `m` must have the same
result parameter where the result is written.


### BMv2 timestamp implementation notes

All timestamps in BMv2 as of 2020-Jun-14 are in units of
microseconds, and they all begin at 0 when the process begins,
e.g. when `simple_switch` or `simple_switch_grpc` begins.

Thus if one starts multiple `simple_switch` processes, either on the
same system, or different systems, it is highly unlikely that their
timestamps will be within 1 microsecond of each other.  Timestamp
values from switch #1 could easily be X microseconds later than, or
earlier than, switch #2, and this time difference between pairs of
switches could change from one time of starting a multi-switch system,
versus another time.  While it is possible to start pairs of
`simple_switch` processes where X would be in the range -10 to +10
microseconds, that would almost certainly be a lucky and unlikely
chance that leads to such close timestamps.  Differences within plus
or minus 2,000,000 microseconds should be fairly straightforward to
achieve, if you go to at least a little bit of effort to start the
processes at the same second as each other.

Many commercial switch and NIC vendors implement standards like PTP
([Precision Time
Protocol](https://en.wikipedia.org/wiki/Precision_Time_Protocol)), but
`simple_switch` does not.

If one wants to use `simple_switch` and have some form of more closely
synchronized timestamps between different switches, you have several
options:

+ Modify `simple_switch` code so that the `ingress_global_timestamp`
  and other timestamp values are calculated from the local system
  time, e.g. perhaps using a system call like `gettimeofday`.  Many
  systems today use NTP ([Network Time
  Protocol](https://en.wikipedia.org/wiki/Network_Time_Protocol)) to
  synchronize their time to some time server, and according to the
  Wikipedia article it can achieve better than one millisecond
  accuracy on local area networks under ideal conditions, and to
  within tens of milliseconds over the public Internet.  If your use
  case can accomodate this much difference in the timestamps on
  different switches, this method should require fairly low
  development effort to implement.
+ Implement PTP in some combination of P4 program and control plane
  software, and run that on simple_switch.  This is likely to be a
  significant amount of time and effort to achieve, and given the
  variable latencies in software processing of packets in virtual
  and/or physical Ethernet interfaces on typical hosts, it seems
  unlikely that synchronization closer than tens or hundreds of
  microseconds would be achievable.  Commercial PTP implementations
  can achieve more tight synchronization because the PTP packet
  handling is implemented within the Ethernet packet processing
  hardware, very close to the physical transmission and reception of
  the data between switches.
+ Implement a newer synchronization algorithm like
  [HUYGENS](https://www.usenix.org/system/files/conference/nsdi18/nsdi18-geng.pdf).
  This is likely to be at least as much development effort as
  implementing PTP, but seems likely to be able to achieve tighter
  time synchronization.
