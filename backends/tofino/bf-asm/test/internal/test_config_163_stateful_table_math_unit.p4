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
    width : 32;
    direct : match_cntr_1;
    //attributes: signed, saturating;
    /* instance_count : 8192; */
}

blackbox stateful_alu cntr_1 {
    reg: stateful_cntr_1;
    condition_lo: pkt.field_e_16 == 7;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo + 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_hi ^ math_unit;

    math_unit_input: pkt.field_f_16;
    math_unit_output_scale: 4;
    math_unit_exponent_shift: -1;
    math_unit_exponent_invert: True;
    math_unit_lookup_table: 0xf  14  13 0xc  0x0b  10 9 8 7 6 5 4 3 2 1 0;

    output_predicate: condition_lo;
    output_value : alu_lo;
    output_dst : meta.needs_sampling;   
}

action cnt_1() {
    /* cntr_1.execute_stateful_alu(pkt.field_i_8); */
    cntr_1.execute_stateful_alu();
}

table match_cntr_1 {
    reads {
        pkt.field_a_32 : exact;
        pkt.field_e_16 : exact;
        pkt.field_f_16 : exact;
        meta.needs_sampling: exact;
    }
    actions {
        cnt_1;
    }
    size : 16384;
}

/* Main control flow */

control ingress {
    apply(match_cntr_1);
}
