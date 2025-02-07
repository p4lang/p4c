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
        pred_4 : 4;
        pad_4 : 4;
        pad_6 : 6;
        comb_pred_1 : 1;
        pad_1 : 1;
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


register r_pred {
    width : 8;
    static : t_pred;
    instance_count : 1024;
}

blackbox stateful_alu b_pred {
    reg: r_pred;
    condition_lo : 1;
    update_lo_1_value: register_lo + 1;

    //output_predicate : condition_lo;
    output_value : predicate;
    output_dst : meta.pred_4;
}

blackbox stateful_alu b_comb_pred {
    reg: r_pred;
    condition_lo : register_lo > 0;
    update_lo_1_value: register_lo + 2;

    output_predicate : condition_lo;
    output_value : combined_predicate;
    output_dst : meta.comb_pred_1;
}

action a_pred(idx){
    b_pred.execute_stateful_alu(idx);
}

action a_comb_pred(idx){
    b_comb_pred.execute_stateful_alu(idx);
}

table t_pred {
    reads {
        pkt.field_a_32 : lpm;
    }
    
    actions {
        a_pred;
        a_comb_pred;
    }
    size : 512;
}

control ingress {
    apply(t_pred);
}
