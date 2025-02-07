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
        color_0 : 8;
        pad_0 : 24;
        pad_1 : 160;
        pad_2 : 24;
        color_1 : 8;
        pad_3 : 24;

        lpf : 16;
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
    modify_field(pkt.field_a_32, param0);
    drop();
    add(pkt.field_b_32, pkt.field_a_32, 2);
}

action action_1(param0){
    drop();
    add(pkt.field_b_32, pkt.field_a_32, 2);
    modify_field(pkt.field_a_32, param0);
}

action action_2(param0){
    drop();
    modify_field(pkt.field_a_32, param0);
    add(pkt.field_b_32, pkt.field_c_32, pkt.field_a_32);
}



table table_0 {
    reads {
        pkt.field_i_8 : exact;
    }
    actions {
        //action_0;
        action_1;
        //action_2;
    }
    size : 256;
}



/* Main control flow */

control ingress {
      apply(table_0);
}
