#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

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


counter cntr_0 {
    type: packets;
    instance_count: 500;
    //min_width: 32;
}

action do_nothing(){}

action action_0() {
    count(cntr_0, pkt.field_i_8);
}


table table_0 {
    actions {
        action_0;
    }
    //size : 256;
}




/* Main control flow */

control ingress {
    apply(table_0);
}
