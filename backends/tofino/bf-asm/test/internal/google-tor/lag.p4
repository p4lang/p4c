// lag.p4

//-----------------------------------
// LAG egress port selection
//-----------------------------------

action set_lag_egress_port(port) {
  modify_field(standard_metadata.egress_spec, port);
}

field_list lag_hash_fields {
  ethernet.dstAddr;
  ethernet.srcAddr;
  ethernet.etherType;
  standard_metadata.ingress_port;
}

field_list_calculation lag_hash {
    input {
        lag_hash_fields;
    }
    algorithm : crc16;
    output_width : 14;
}

action_selector lag_selector {
    selection_key : lag_hash;
    selection_mode : fair;
}

action_profile lag_action_profile {
  actions {
    set_lag_egress_port;
  }
  dynamic_action_selection: lag_selector;
}

table lag_resolve_table {
  reads {
    standard_metadata.egress_spec: exact;
  }
  action_profile: lag_action_profile;
}

control lag_handling {
  apply(lag_resolve_table);
}

