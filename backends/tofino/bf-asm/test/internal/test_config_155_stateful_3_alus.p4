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

header_type meta_t {
     fields {
         count_value : 16;
         needs_sampling : 8;
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

register stateful_cntr_1{
    width : 16;
    direct : match_cntr_1;
    /* instance_count : 8192; */
}

register stateful_cntr_2{
    width : 16;
    direct : match_cntr_2;
    /* instance_count : 8192; */
}

register flow_cnt {
    width : 8;
    direct : match_flow;
    /* instance_count : 65536; */
}

blackbox stateful_alu cntr_1 {
    reg: stateful_cntr_1;
    update_lo_1_value: register_lo + 1;
}

blackbox stateful_alu cntr_2 {
    reg: stateful_cntr_2;
    update_lo_1_value: register_lo + 1;
    output_value : alu_lo;
    output_dst : meta.count_value;
}

blackbox stateful_alu sampler_alu {
    reg: flow_cnt;
    condition_lo: register_lo == 10;  /* sample limit */
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_lo + 1;
    output_predicate: condition_lo;
    output_value : alu_lo;
    output_dst : meta.needs_sampling;
}


action cnt_1() {
    cntr_1.execute_stateful_alu();
}

action cnt_2() {
    cntr_2.execute_stateful_alu(); 
}

action sample(){
    sampler_alu.execute_stateful_alu();
}

@pragma table_counter disabled
table match_cntr_1 {
    reads {
        pkt.field_a_32 : exact;
    }
    actions {
        cnt_1;
    }
    size : 16384;
}

table match_cntr_2 {
    reads {
        pkt.field_a_32 : exact;
        pkt.field_j_8 : exact;
        meta.count_value : exact;
    }
    actions {
        cnt_2;
    }
    size : 16384;
}

table match_flow {
    reads {
        pkt.field_a_32 : ternary;
        meta.needs_sampling : exact;  /* hack to ensure phv puts it in mau space */
    }
    actions {
        sample;
    }
    size : 8192;
}

/* Main control flow */

control ingress {
    apply(match_cntr_1);
    apply(match_cntr_2);
    apply(match_flow);
}
