#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_28 : 28;
        field_a2_4 : 4;
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
        field_l_8 : 8;
    }
}

parser start {
    return parse_pkt;
}

header pkt_t pkt;

parser parse_pkt {
    extract(pkt);
    return ingress;
}


action action_0(param0){
    modify_field(pkt.field_b_32, param0);
}

action action_1(param0){
    modify_field(pkt.field_c_32, param0);
}

action action_2(param0){
    modify_field(pkt.field_d_32, param0);
}

action do_nothing(){
    no_op();
}

table table_0 {
    reads {
        pkt.field_e_16 : exact;
    }
    actions {
        action_0;
    }
    size : 200000;
}

table table_1 {
    reads {
        pkt.field_f_16 : exact;
    }
    actions {
        action_1;
    }
}

table table_2 {
    reads {
        pkt.field_g_16 : exact;
    }
    actions {
        action_2;
    }
}


/* Main control flow */

control ingress {
    if (valid(pkt)){
        apply(table_0);
        apply(table_1);
    }
/*
    if (valid(pkt)){
      apply(table_1);
    }
*/
    apply(table_2);
}
