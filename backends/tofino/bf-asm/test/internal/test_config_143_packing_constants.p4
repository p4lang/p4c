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

        field_i_bit_7 : 1;
        field_i_bit_6 : 1;
        field_i_bit_5 : 1;
        field_i_bit_4 : 1;
        field_i_bit_3 : 1;
        field_i_bit_2 : 1;
        field_i_bit_1 : 1;
        field_i_bit_0 : 1;

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

action set_hi(){
    modify_field(pkt.field_i_bit_7, 1);
    modify_field(pkt.field_i_bit_6, 0);
    modify_field(pkt.field_i_bit_5, 1);
}

action set_mid(){
    modify_field(pkt.field_i_bit_5, 1);
    modify_field(pkt.field_i_bit_4, 0);
    modify_field(pkt.field_i_bit_3, 1);
}

action set_lo(){
    modify_field(pkt.field_i_bit_2, 1);
    modify_field(pkt.field_i_bit_1, 0);
    modify_field(pkt.field_i_bit_0, 1);
}



table table_0 {
    reads {
        pkt.field_a_32 : ternary;
    }
    actions {
        set_hi;
        set_mid;
        set_lo;
    }
    size : 512;
}



/* Main control flow */

control ingress {
    apply(table_0);
}
