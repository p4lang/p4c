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

        field_i_16 : 16;
        field_j_16 : 16;
        field_k_8 : 8;
        field_l_8 : 8;

    }
}

header_type meta_t {
     fields {
         needs_sampling : 16 (signed, saturating);
     }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;
metadata meta_t meta;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

@pragma stateful_table_counter table_hit
register flow_cnt {
    width : 16;
    direct : match_tbl;
    /* instance_count : 65536; */
}

blackbox stateful_alu sampler_alu {
    reg: flow_cnt;
    condition_hi: pkt.field_i_16 == 3;  /* testing */
    condition_lo: register_lo == 10;  /* sample limit */
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value:  register_lo + 1;
    update_hi_1_value: pkt.field_j_16 + 2;
    output_predicate: condition_lo;
    output_value : alu_lo;
    output_dst : meta.needs_sampling;
}

action sample(){
    sampler_alu.execute_stateful_alu();
}

action do_nothing(){}

table match_tbl {
    reads {
        pkt.field_a_32 : ternary;
        pkt.field_i_16 : exact;  /* hack */
        pkt.field_j_16 : exact;  /* hack */
        meta.needs_sampling : exact;  /* hack to ensure phv puts it in mau space */
    }
    actions {
        sample;
    }
    size : 4096;
}




/* Main control flow */

control ingress {
    apply(match_tbl);
}
