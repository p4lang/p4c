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


action action_0(param0, param1){
    modify_field(pkt.field_c_32, param0);
    modify_field(pkt.field_g_16, param1);
}

action action_1(){
    drop();
}


//@pragma immediate 0
@pragma proxy_hash_width 24
@pragma proxy_hash_algorithm crc16_extend
table table_0 {
    reads {
        pkt.field_a_32 : exact;
        pkt.field_b_32 : exact;
        pkt.field_e_16 : exact;
        pkt.field_f_16 : exact;
    }
    actions {
        action_0;
        action_1;
    }
    size : 16384;
}



/* Main control flow */

control ingress {
      apply(table_0);
}
