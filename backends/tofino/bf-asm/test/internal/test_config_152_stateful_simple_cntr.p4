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

register stateful_cntr{
    width : 16;
    /* direct : match_cntr; */
    instance_count : 8192;
}


blackbox stateful_alu cntr {
    reg: stateful_cntr;
    update_lo_1_value: register_lo + 1;
}


action cnt() {
    cntr.execute_stateful_alu();
}

action do_nothing(){}

table match_cntr {
    reads {
        pkt.field_a_32 : exact;
    }
    actions {
        cnt;
        do_nothing;
    }
    size : 16384;
}



/* Main control flow */

control ingress {
    apply(match_cntr);
}
