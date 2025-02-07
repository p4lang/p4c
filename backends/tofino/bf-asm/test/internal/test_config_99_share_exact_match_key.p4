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
    modify_field(pkt.field_d_32, param0);
}

action action_1(param0){
    modify_field(pkt.field_h_16, param0);
}

action do_nothing(){
    no_op();
}

table table_0 {
    reads {
        pkt.field_a_32 : ternary;
        pkt.field_b_32 : exact;
        pkt.field_e_16 : exact;
        pkt.field_f_16 : exact;
        pkt.field_i_8  : exact;
    }
    actions {
        action_0;
    }
    size : 512;
    //size : 4096;
}

@pragma include_idletime 1
table table_1 {
    reads {
        pkt.field_a_32 : exact;
        pkt.field_e_16 : exact;
        pkt.field_b_32 : exact;
        pkt.field_f_16 : exact;
        //pkt.field_i_8  : exact;

        pkt.field_c_32 : exact;
        pkt.field_g_16 : exact;
        pkt.field_j_8  : exact;
        pkt.field_k_8  : exact;

    }
    actions {
        action_1;
    }
    size : 1024;
    //size : 8192;
}

/* Main control flow */

control ingress {
    if (valid(pkt)){
        apply(table_0);
        apply(table_1);
    }
}
