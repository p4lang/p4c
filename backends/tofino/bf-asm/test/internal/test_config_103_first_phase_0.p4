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
        field_a0_1 : 1;
        field_a1_1 : 1;
        field_a2_1 : 1;
        field_a3_1 : 1;
        field_b_4 : 4;
        field_c_4 : 4;
        field_d_4 : 4;
        field_e_4 : 4;
        field_f_4 : 4;
        field_g_4 : 4;
        field_h0_1 : 1;
        field_h1_1 : 1;
        field_h2_1 : 1;
        field_h3_1 : 1;
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


action action_0(param0, param1, param2, param3, param4, param5){
    modify_field(meta.field_a0_1, 0);
    modify_field(meta.field_a1_1, param0);
    modify_field(meta.field_a2_1, 1);
    modify_field(meta.field_a3_1, param1);
    modify_field(meta.field_b_4, 9);
    modify_field(meta.field_c_4, param2);
    modify_field(meta.field_d_4, 6);
    modify_field(meta.field_e_4, 10);
    modify_field(meta.field_f_4, 15, 0xf);
    modify_field(meta.field_g_4, param3);
    modify_field(meta.field_h0_1, param4);
    modify_field(meta.field_h1_1, 0);
    modify_field(meta.field_h2_1, param5);
    modify_field(meta.field_h3_1, 1);
}

action action_1(param0){
    modify_field(pkt.field_f_16, param0);
}

action do_nothing(){
    no_op();
}

table table_0 {
    reads {
         ig_intr_md.ingress_port  : exact;
    }
    actions {
        action_0;
    }
    size : 128;
}

@pragma include_idletime 1
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table table_1 {
    reads {
        pkt.field_g_16 : exact;
        pkt.color_0 : exact;

        meta.field_a0_1 : exact;
        meta.field_a1_1 : exact;
        meta.field_a2_1 : exact;
        meta.field_a3_1 : exact;

        meta.field_b_4 : exact;
        meta.field_c_4 : exact;
        meta.field_d_4 : exact;
        meta.field_e_4 : exact;
        meta.field_f_4 : exact;
        meta.field_g_4 : exact;

        meta.field_h0_1 : exact;
        meta.field_h1_1 : exact;
        meta.field_h2_1 : exact;
        meta.field_h3_1 : exact;

    }
    actions {
        action_1;
    }
    size : 65536;
}

/* Main control flow */

control ingress {
    if (0 == ig_intr_md.resubmit_flag){
        apply(table_0);
        apply(table_1);
    }
}
