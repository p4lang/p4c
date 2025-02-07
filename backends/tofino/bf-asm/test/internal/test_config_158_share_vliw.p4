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
    add(pkt.field_a_32, pkt.field_b_32, pkt.field_c_32);
    modify_field(pkt.field_j_8, 3);
}

action action_1(){
    subtract(pkt.field_a_32, pkt.field_b_32, pkt.field_c_32);
    shift_left(pkt.field_e_16, pkt.field_e_16, 5);
}

action do_nothing(){ no_op();}

table table_0 {
    reads {
        pkt.field_b_32 : ternary;
    }
    actions {
        action_0;
        action_1;
        do_nothing;
    }
    size : 512;
}

table table_1 {
    reads {
        pkt.field_c_32 : ternary;
    }
    actions {
        action_0;
        action_1;
        do_nothing;
    }
    size : 512;
}


/* Main control flow */

control ingress {
    if (pkt.field_i_8 == 1){
        apply(table_0);
    } else {
        apply(table_1);
    }
}
