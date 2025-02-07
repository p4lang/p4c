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
    return parse_ethernet;
}

header pkt_t pkt;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

action action_0(){
    shift_left(pkt.field_i_8, pkt.field_j_8, 2);
}

action action_1(){
    shift_left(pkt.field_i_8, 1, 2);
}

action action_2(){
    shift_right(pkt.field_i_8, pkt.field_j_8, 5);
}

action action_3(){
    shift_right(pkt.field_i_8, 128, 5);
}

table table_0 {
    reads {
        pkt.field_a_32 : ternary;
    }
    actions {
        action_0;
        action_1;
        action_2;
        action_3;
    }
    size : 512;
}



/* Main control flow */

control ingress {
    apply(table_0);
}
