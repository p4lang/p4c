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
TODO: mc_dump
TODO: mc_mgrp_create
TODO: mc_mgrp_destroy
TODO: mc_node_associate
TODO: mc_node_create
TODO: mc_node_destroy
TODO: mc_node_dissociate
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
