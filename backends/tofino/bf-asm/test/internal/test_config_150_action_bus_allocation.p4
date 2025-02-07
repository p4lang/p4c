#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_signed : 32 (signed);
        field_b_32 : 32;
        field_c_32 : 32;
        field_d_32 : 32;

        field_e_sat : 16 (saturating);
        field_f_16 : 16;
        field_g_16 : 16;
        field_h_16 : 16;

        field_i_signed_sat : 8 (signed, saturating);
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


action action_0(param0){
    add(pkt.field_a_signed, pkt.field_a_signed, param0);
}

action action_1(param0) {
    add_to_field(pkt.field_e_sat, param0);
}

action action_2(param0) {
    add(pkt.field_i_signed_sat, pkt.field_i_signed_sat, param0);
}

action action_3(param0) {
    modify_field(pkt.field_c_32, param0);
}

action action_4(param0) {
    modify_field(pkt.field_g_16, param0);
}

action action_5(param0) {
    modify_field(pkt.field_k_8, param0);
}


table table_0 {
    reads {
        pkt.field_c_32 : ternary;
    }
    actions {
        action_0;
        action_1;
        action_2;
    }
    size : 512;
}

table table_1 {
    reads {
        pkt.field_d_32 : ternary;
    }
    actions {
        action_3;
        action_4;
        action_5;
    }
    size : 512;
}


/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
}
