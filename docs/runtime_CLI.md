```
TODO: act_prof_add_member_to_group
TODO: act_prof_create_group
TODO: act_prof_create_member
TODO: act_prof_delete_group
TODO: act_prof_delete_member
TODO: act_prof_dump
TODO: act_prof_dump_group
TODO: act_prof_dump_member
TODO: act_prof_modify_member
TODO: act_prof_remove_member_from_group
TODO: counter_read
TODO: counter_reset
TODO: help
TODO: load_new_config_file
```

### mc_dump, mc_mgrp_create, mc_mgrp_destroy, mc_node_associate, mc_node_create, mc_node_destroy, mc_node_dissociate

When ingress processing is complete in the v1model architecture, the
ingress P4 code can assign values to a few fields in standard_metadata
that control whether the packet is dropped, sent via unicast to one
output port, or multicast replicated to a list of 0 or more output
ports.  See
[here](https://github.com/p4lang/behavioral-model/blob/master/docs/simple_switch.md#pseudocode-for-what-happens-at-the-end-of-ingress-and-egress-processing)
for more details on how `simple_switch` decides between these
alternatives.

The packet buffer contains a PRE (Packet Replication "Engine").

The `v1model.p4` standard_metadata field `mcast_grp` specifies one of
65,535 multicast group id values, in the range `[1, 65535]`.
`mcast_grp` equal to 0 means not to multicast replicate the packet.
simple_switch can use a larger range of multicast group ids.  The
range given here is limited by the definition of the `mcast_grp`
standard_metadata field in [p4c's `v1model.p4` include
file](https://github.com/p4lang/p4c/blob/master/p4include/v1model.p4),
which has type `bit<16>`.

For packets that should be multicast replicated, the PRE looks up the
`mcast_grp` value of the packet in a table, and gets back a list of 0
or more `(egress_port, egress_rid)` pairs for what copies to make.
Each copy will independently later begin egress processing with those
two standard_metadata fields initialized to that pair of values.

Note: The multicast table described above is not a P4 table that you
define in your P4 program.  Here "table" is meant in the more general
sense of the word.  It is similar to a P4 table in that the
`mcast_grp` field is the only search key field, and it is always exact
match.  It is dissimilar in that:

* You do not declare the table in your program.  It exists in
  simple_switch no matter what P4 program you load into it, and
  whether you use it or not.
* The control plane software uses different API calls to read and
  write the multicast group table than it does for P4 tables.
* The result of the lookup is a variable-length list of pairs, whereas
  the P4 language has no straightforward way to represent a run-time
  variable length list of values.

There are effectively two "levels" of objects involved in managing
multicast replication lists:

* Each multicast group is a set of multicast _nodes_.
* Each multicast node that you create has one `egress_rid` value, and
  a set of `egress_port` values, which may not contain any duplicates.

Suppose you want to configure simple_switch such that packets to be
multicast with a `mcast_grp` value of 1113 will be replicated to this
list of `(egress_port, egress_rid)` pairs:

* (egress_port=0, egress_rid=5)
* (egress_port=7, egress_rid=10)
* (egress_port=3, egress_rid=5)

Because the first and last copies both have the same `egress_rid`
value 5, so they could be in the same multicast node, which is what we
will do in this example.  Create a multicast node with the
`mc_node_create` command:

```
RuntimeCmd: help mc_node_create
Create multicast node: mc_node_create <rid> <space-separated port list> [ | <space-separated lag list> ]

RuntimeCmd: mc_node_create 5 0 3
Creating node with rid 5 , port map 1001 and lag map
node was created with handle 0
```

Note the `handle 0` in the output.  Every time you create a multicast
node, simple_switch at run time chooses an integer id or handle to
"name" it, and that node handle must be used when referring to that
node in other commands below.  The values are typically assigned from
0 on up as new nodes are created, but it is better if you do not rely
on this detail.

Create another multicast node for the copies to make with `egress_rid`
of 10, which in this example is only port 7:

```
RuntimeCmd: mc_node_create 10 7
Creating node with rid 10 , port map 10000000 and lag map
node was created with handle 1
```

The output shows that this node was allocated a node handle of 1.

Create the multicast group for group id 1113 with the `mc_mgrp_create`
command:

```
RuntimeCmd: help mc_mgrp_create
Create multicast group: mc_mgrp_create <group id>

RuntimeCmd: mc_mgrp_create 1113
Creating multicast group 1113
```

There are no handle ids for multicast groups.  Groups are always named
by the multicast group id that you select, which is 1113 in this
example.

We can use the `mc_dump` command to see all created groups:

```
RuntimeCmd: mc_dump
==========
MC ENTRIES
**********
mgrp(1113)
==========
LAGS
==========
```

We see that the group with id 1113 exists, but there are no multicast
nodes associated with it.  Later examples below show what the output
looks like when a multicast group does have nodes associated with it.
In the current configuration of simple_switch, sending a packet to be
replicated with `mcast_grp` 1113 would cause the packet to be
replicated 0 times, which is effectively the same as dropping it.

To add a multicast node to a multicast group, we use the
`mc_node_associate` command:

```
RuntimeCmd: help mc_node_associate
Associate node to multicast group: mc_node_associate <group handle> <node handle>

RuntimeCmd: mc_node_associate 1113 0
Associating node 0 to multicast group 1113

RuntimeCmd: mc_dump
==========
MC ENTRIES
**********
mgrp(1113)
  -> (L1h=0, rid=5) -> (ports=[0, 3], lags=[])
==========
LAGS
==========
```

Here we see that group 1113 has one node associated with it.  `L1h` is
an abbreviation for `L1 handle`, which you may see in the
simple_switch log messages, and means the same thing as a node handle.
We see the rid value of 5, where rid is an abbrevation of
`egress_rid`, and we see the list of ports 0 and 3.

In the current configuration of simple_switch, sending a packet to be
replicated with `mcast_grp` 1113 would cause the packet to be
replicated 2 times, with these `(egress_port, egress_rid)` values when
they begin egress processing:

* (egress_port=0, egress_rid=5)
* (egress_port=3, egress_rid=5)

To reach the goal we originally set for ourselves, we now add the
second node to the group 1113:

```
RuntimeCmd: mc_node_associate 1113 1
Associating node 1 to multicast group 1113

RuntimeCmd: mc_dump
==========
MC ENTRIES
**********
mgrp(1113)
  -> (L1h=0, rid=5) -> (ports=[0, 3], lags=[])
  -> (L1h=1, rid=10) -> (ports=[7], lags=[])
==========
LAGS
==========
```

In the current configuration of simple_switch, sending a packet to be
replicated with `mcast_grp` 1113 would cause the packet to be
replicated 3 times, with these `(egress_port, egress_rid)` values when
they begin egress processing:

* (egress_port=0, egress_rid=5)
* (egress_port=3, egress_rid=5)
* (egress_port=7, egress_rid=10)

We can dissociate a node from a group using the `mc_node_dissociate`
command:

```
RuntimeCmd: help mc_node_dissociate
Dissociate node from multicast group: mc_node_associate <group handle> <node handle>

RuntimeCmd: mc_node_dissociate 1113 0
Dissociating node 0 from multicast group 1113

RuntimeCmd: mc_dump
==========
MC ENTRIES
**********
mgrp(1113)
  -> (L1h=1, rid=10) -> (ports=[7], lags=[])
==========
LAGS
==========
```

In the current configuration of simple_switch, sending a packet to be
replicated with `mcast_grp` 1113 would cause the packet to be
replicated 1 time, with these `(egress_port, egress_rid)` values when
they begin egress processing:

* (egress_port=7, egress_rid=10)

If we then dissociate node 1 from group 1113, it returns to its
initially created state where it will cause 0 copies to be made.

```
RuntimeCmd: mc_node_dissociate 1113 1
Dissociating node 1 from multicast group 1113

RuntimeCmd: mc_dump
==========
MC ENTRIES
**********
mgrp(1113)
==========
LAGS
==========
```

You may destroy a multicast group with the `mc_mgrp_destroy` command:

```
RuntimeCmd: help mc_mgrp_destroy
Destroy multicast group: mc_mgrp_destroy <group id>

RuntimeCmd: mc_mgrp_destroy 1113
Destroying multicast group 1113

RuntimeCmd: mc_dump
==========
MC ENTRIES
==========
LAGS
==========
```

There are some restrictions on these operations:

* To associate a node to a group, both the group and the node must
  have already been created.  It does not matter which one was created
  first.
* simple_switch only allows a node to be associated with one group at
  a time.  You may dissociate a node from one group, then associate it
  with a different group, if you wish.
* It is recommended that if you wish to destroy a group, that you
  first dissociate all nodes from it.
* A single node cannot send more than one copy to the same
  egress_port.  If you wish for replication to create multiple copies
  with the same egress_port, use multiple nodes.  Using different
  egress_rid values for these nodes enables your egress P4 code to
  distinguish the copies from each other, in case you want to process
  them differently.

The `mc_dump` command shows details about multicast nodes that have
been associated with a group.  There is no simple_switch_CLI command
that shows details about multicast nodes that have not been associated
with a group.

Note: The simple_switch_CLI help documentation and the output of
`mc_dump` mentions `lag` and `lag lists` as well, but those are not
documented here yet.  You can use multicast replication perfectly well
without them.


```
TODO: mc_node_update
TODO: mc_set_lag_membership
TODO: meter_array_set_rates
TODO: meter_get_rates
```

### meter_set_rates

The syntax for the CLI is provided below.

```
RuntimeCmd: help meter_set_rates
Configure rates for a meter: meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ...
```

Meter behavior is specified in [RFC 2697](https://tools.ietf.org/html/rfc2697)
and [RFC 2698](https://tools.ietf.org/html/rfc2698).

The user enters the meter rate in bytes/microsecond and burst_size in bytes.  If
the meter type is packets, the rate is entered in packets/microsecond and
burst_size is the number of packets.


```
TODO: port_add
TODO: port_remove
TODO: register_read
TODO: register_reset
TODO: register_write
TODO: reset_state
TODO: serialize_state
TODO: set_crc16_parameters
TODO: set_crc32_parameters
TODO: shell
```

### show_actions

No parameters.  For every action in the currently loaded P4 program, shows the
name and list of parameters.  For each parameter, the name and width in bits is
shown.


### show_ports

No parameters.  Shows the port numbers, the interface names they are associated
with, and their status (e.g. up or down).


### show_tables

No parameters.  For every table in the currently loaded P4 program, shows the
name, implementation (often `None`, but can be an action profile or action
selector for tables created with those options), and a list of table search key
fields, giving for each such field its name, match kind (e.g. `exact`, `lpm`,
`ternary`, `range`), and width in bits.


```
TODO: swap_configs
TODO: switch_info
```


### table_add

```
RuntimeCmd: help table_add
Add entry to a match table: table_add <table name> <action name> <match fields> => <action parameters> [priority]
```

You can find a list of all table and action names via the commands `show_tables`
and `show_actions`.  The default name created if you do not use a `@name`
annotation to specify one in your P4 program is often hierarchical, starting
with the name of a control that the table or action is defined within.

Match fields must appear in the same order as the table search key fields are
specified in the P4 program, without names.  The output of `show_tables` also
shows the names, match kinds (`ternary`, `range`, `lpm`, or `exact`), and bit
widths for every table, after the `mk=` string.

Numeric values can be specified in decimal, or hexadecimal prefixed with `0x`.

There are some special syntaxes accepted to specify numeric values for the
convenience of some common network address formats:

* 32-bit values can be specified like IPv4 addresses, in dotted decimal,
  e.g. `10.1.2.3`
* 128-bit values can be specified like IPv6 addresses in IETF notation,
  e.g. `FEDC:BA98:7654:3210:FEDC:BA98:7654:3210` or `1080::8:800:200C:417A`.
  See [RFC 2732](https://www.ietf.org/rfc/rfc2732.txt).
* 48-bit values can be specified like Ethernet MAC addresses, with each byte in
  two hexadecimal digits, with bytes separated by `:`, e.g. `00:12:34:56:78:9a`.

Table search key fields with match kind `lpm` should have the value followed by
the prefix length after a slash character `/`, e.g. `0x0a010203/24`.  The prefix
length must always be in decimal.  Hexadecimal is not supported for prefix
lengths.  A prefix length of 0 may be used, and such an entry will match any
value of that search key field.

Table search key fields with match kind `ternary` are specified as a numeric
value, then `&&&`, then a numeric mask, with no white space between the numbers
and the `&&&`.  Bit positions of the mask equal to 1 are exact match bit
positions, and bit positions of the mask equal to 0 are wildcard bit positions.
That is, the table entry `value&&&mask` will match the search key value `k` if
`(k & mask) == (value & mask)`, where `&` is bit-wise logical and like in P4.
To match any value of a ternary search key, you can specify `0&&&0`.

Table search key fields with match kind `range` are specified as a minimum
numeric value, then `->`, then a maximum numeric value, with no white space
between the numbers and the `->`.  To match any value of a range search key, you
can specify `0->255` for an 8-bit field, or `0->0xffffffff` for a 32-bit field,
etc.

If any of the search key fields of a table have match kind `ternary` or `range`,
then a numeric `priority` value must be specified.  For any other kind of table,
it is an error to specify a `priority` value.  If a search key matches multiple
table entries, then among the ones that match the search key, one with the
smallest numeric priority value will be the winner, meaning that its action will
be executed.

Action parameters must appear in the same order as they appear in the P4
program, without names.  The output of the `show_actions` command shows the name
and width in bits of all parameters of every action.


```
TODO: table_clear
TODO: table_delete
TODO: table_dump
TODO: table_dump_entry
TODO: table_dump_entry_from_key
TODO: table_dump_group
TODO: table_dump_member
TODO: table_indirect_add
TODO: table_indirect_add_member_to_group
TODO: table_indirect_add_with_group
TODO: table_indirect_create_group
TODO: table_indirect_create_member
TODO: table_indirect_delete
TODO: table_indirect_delete_group
TODO: table_indirect_delete_member
TODO: table_indirect_modify_member
TODO: table_indirect_remove_member_from_group
TODO: table_indirect_reset_default
TODO: table_indirect_set_default
TODO: table_indirect_set_default_with_group
TODO: table_info
TODO: table_modify
TODO: table_num_entries
```


### table_reset_default

```
RuntimeCmd: help table_reset_default
Reset default entry for a match table: table_reset_default <table name>
```

Changes the action executed by a table when a miss occurs back to its original
value, as specified in the P4 program.  If no default action is specified in the
P4 program, it is a no-op action, sometimes called `NoAction`.


### table_set_default

```
RuntimeCmd: help table_set_default
Set default action for a match table: table_set_default <table name> <action name> <action parameters>
```

Assign a different action to be the default action of the named table, i.e. the
action executed when the table is applied, but the search key does not match any
entries installed in the table.

See documentation for the `table_add` command for how action parameters are
specified.


```
TODO: table_set_timeout
TODO: table_show_actions
TODO: write_config_to_file
```
