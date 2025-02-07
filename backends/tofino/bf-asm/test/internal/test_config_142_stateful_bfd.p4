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

header_type bfd_t {
    fields {
       bfd_tx_or_rx : 8;
       bfd_discriminator : 8;
    }
}

header_type sample_t {
     fields {
         a : 8 ;
         b : 8 ;
     }
}


parser start {
    return parse_ethernet;
}

header pkt_t pkt;
metadata bfd_t bfd_md;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

register bfd_cnt{
    /* 
    width : 8;
    */
    layout : sample_t;
    direct : bfd;
    /* instance_count : 16384; */
}

/*
stateful_alu bfd_cnt_rx_alu {
    register: bfd_cnt;
    update_lo_1_value: 0;
}
stateful_alu bfd_cnt_tx_alu {
    register: bfd_cnt;
    condition_a: register > 3;
    update_hi_1_value: 1;
    update_lo_1_value: register + 1;
    output_predicate: a;
    output_expr: new_hi;
    output_dst: egress_md.bfd_timeout_detected;
}
*/

action bfd_rx() {
    /* execute_stateful_alu(bfd_cnt_rx_alu); */
}

action bfd_tx() {
    /* execute_stateful_alu(bfd_cnt_tx_alu); */
}


table bfd {
    reads {
        bfd_md.bfd_tx_or_rx : exact;
        bfd_md.bfd_discriminator : exact;
    }
    actions {
        bfd_rx;
        bfd_tx;
    }
    size : 16384;
}



/* Main control flow */

control ingress {
    apply(bfd);
}
