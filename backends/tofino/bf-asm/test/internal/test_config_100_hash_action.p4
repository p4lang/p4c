#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_32 : 32;
        field_b_32 : 32;
        field_c_32 : 32;
        field_d_32 : 32;

        field_e_16 : 16;
        field_f_16 : 16;
        field_g_16 : 16;
        field_h_16 : 16;

        field_i_8 : 8;
        field_j_8 : 8;
        field_k_8 : 8;
        color_0 : 8;
    }
}

header_type meta_t {
    fields {
        field_17 : 17;
        pad_15   : 15;
    }
}

parser start {
    return parse_pkt;
}

header pkt_t pkt;
metadata meta_t meta;

parser parse_pkt {
    extract(pkt);
    return ingress;
}


action action_0(param0){
    modify_field(pkt.field_c_32, param0);
}

action action_1(param0){
    modify_field(pkt.field_f_16, param0);
}

action action_2(){
    count(simple_stats, meta.field_17);
}

action do_nothing(){
    no_op();
}

counter simple_stats {
     type : packets;
     instance_count : 32768;
}

@pragma action_default_only do_nothing
table table_0 {
    reads {
         pkt.field_i_8  : exact;
    }
    actions {
        do_nothing;
        action_0;
    }
    size : 256;
}

@pragma include_idletime 1
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table table_1 {
    reads {
        pkt.field_g_16 : exact;
    }
    actions {
        action_1;
    }
    size : 65536;
}

table table_2 {
     actions {
         action_2;
     }
}

/* Main control flow */

control ingress {
    if (valid(pkt)){
        apply(table_0);
        apply(table_1);
        apply(table_2);
    }
}


