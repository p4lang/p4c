#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_32 : 32;
        field_b_32 : 32;
        field_c_32 : 32;
        field_d_32 : 32;

        field_e_16 : 1;
        field_f_16 : 16;
        field_g_16 : 16;
        field_h_16 : 16;

        field_i_16 : 16;
        field_j_16 : 16;
        field_k_8 : 8;
        field_l_8 : 8;

        from_pkt_gen : 1;
        pad_0 : 7;        
    }
}

header_type meta_t {
     fields {
         live_port : 1 (signed, saturating);
         pad_0 : 7;
     }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;
metadata meta_t meta;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

@pragma stateful_table_counter table_hit
register port_liveness_update {
    width : 1;
    /* direct : port_liveness_update_match_tbl; */
    instance_count : 65536;
}

blackbox stateful_alu make_port_live {
    reg: port_liveness_update;
    selector_binding: selector_match_tbl;
    update_lo_1_value: clr_bit;
    output_value : alu_lo;
    output_dst : meta.live_port;
}

action port_alive(){
    make_port_live.execute_stateful_alu();
}

action action_0(param_0){
    modify_field(pkt.field_b_32, param_0);
}

action do_nothing(){}


field_list selector_fields {
         pkt.field_c_32;
         pkt.field_d_32;
}

field_list_calculation selector_hash {
     input {
         selector_fields;
     }
     algorithm : crc16;
     output_width : 16;
}


action_selector some_selector {
     selection_key : selector_hash;
     selection_mode : fair;
}

action_profile selector_match_tbl_action_profile {
    actions {
        action_0;
    }
    size : 1024;
    dynamic_action_selection : some_selector;
}


table selector_match_tbl {
    reads {
        pkt.field_a_32 : ternary;
    }
    action_profile: selector_match_tbl_action_profile;
    size : 4096;
}

table port_liveness_update_match_tbl {
    reads {
        pkt.field_k_8 : ternary;
        meta.live_port : exact;   /* HACK */
    }
    actions {
        port_alive;
    }
    size : 1024;
}

/* Main control flow */

control ingress {
    if (pkt.from_pkt_gen == 0){
        apply(selector_match_tbl);
    } else {
        apply(port_liveness_update_match_tbl);
    }
}
