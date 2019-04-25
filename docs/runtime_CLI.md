###
TODO: Write help for any CLI not preceded by "###".
###

act_prof_add_member_to_group  
act_prof_create_group  
act_prof_create_member  
act_prof_delete_group  
act_prof_delete_member  
act_prof_dump  
act_prof_dump_group  
act_prof_dump_member  
act_prof_modify_member  
act_prof_remove_member_from_group  
counter_read  
counter_reset  
help  
load_new_config_file  
mc_dump  
mc_mgrp_create  
mc_mgrp_destroy  
mc_node_associate  
mc_node_create  
mc_node_destroy  
mc_node_dissociate  
mc_node_update  
mc_set_lag_membership  
meter_array_set_rates  
meter_get_rates  
  
### meter_set_rates  
The syntax for the CLI is provided below.  
`RuntimeCmd: help meter_set_rates  
Configure rates for a meter: meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ...`  
  
Meter behavior is specified in RFC 2698.  

The user enters the meter rate in bytes/microsecond and burst_size in  
bytes.  If the meter type is packets, the rate is entered in packets/microsecond  
and burst_size is the number of packets.  
  
port_add  
port_remove  
register_read  
register_reset  
register_write  
reset_state  
serialize_state  
set_crc16_parameters  
set_crc32_parameters  
shell  
show_actions  
show_ports  
show_tables  
swap_configs  
switch_info  
table_add  
table_clear  
table_delete  
table_dump  
table_dump_entry  
table_dump_entry_from_key  
table_dump_group  
table_dump_member  
table_indirect_add  
table_indirect_add_member_to_group  
table_indirect_add_with_group  
table_indirect_create_group  
table_indirect_create_member  
table_indirect_delete  
table_indirect_delete_group  
table_indirect_delete_member  
table_indirect_modify_member  
table_indirect_remove_member_from_group  
table_indirect_reset_default  
table_indirect_set_default  
table_indirect_set_default_with_group  
table_info  
table_modify  
table_num_entries  
table_reset_default  
table_set_default  
table_set_timeout    
table_show_actions  
write_config_to_file  
