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

action do_nothing(){
    no_op();
}

action action_0(param0) {
    subtract(pkt.field_a_32, param0, pkt.field_a_32);
}

action action_1() {
    subtract(pkt.field_i_8, pkt.field_j_8, pkt.field_k_8);
}

table table_0 {
    reads {
        pkt.field_a_32 : exact;
    }
    action_profile: table_0_action_profile;
    size : 3072;
}

action_profile table_0_action_profile {
    actions {
        do_nothing;
        action_0;
        action_1;
    }
    size : 3072;
}

/* Main control flow */

control ingress {
    apply(table_0);
}
