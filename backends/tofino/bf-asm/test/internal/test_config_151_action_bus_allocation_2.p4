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
    modify_field(pkt.field_b_32, param0, 0x0ff00000);
}

action action_1(param0){
    modify_field(pkt.field_c_32, param0, 0x000ff000);
}

action action_2(param0){
    modify_field(pkt.field_d_32, param0, 0x0ff0);
}

table table_0 {
    reads {
        pkt.field_h_16 : ternary;
    }
    actions {
        action_0;
    }
    size : 512;
}

table table_1 {
    reads {
        pkt.field_h_16 : ternary;
    }
    actions {
        action_1;
    }
    size : 512;
}

table table_2 {
    reads {
        pkt.field_h_16 : ternary;
    }
    actions {
        action_2;
    }
    size : 512;
}


/* Main control flow */

control ingress {
    apply(table_0);
    apply(table_1);
    apply(table_2);
}
