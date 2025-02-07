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
        result_8 : 8;
        result_8_2 : 8;
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


register reg_0 {
    width : 8;
    static : table_0;
    instance_count : 131072;
}

register reg_1 {
    width : 8;
    static : table_1;
    instance_count : 131072;
}

blackbox stateful_alu alu_0 {
    reg: reg_0;
    condition_lo : 1;
    update_lo_1_value: 15;

    output_value : alu_lo;
    output_dst : meta.result_8;
    reduction_or_group: or_group_1;
}

blackbox stateful_alu alu_1 {
    reg: reg_1;
    condition_lo : 1;
    update_lo_1_value: 0x30;

    output_value : alu_lo;
    output_dst : meta.result_8;
    reduction_or_group: or_group_1;
}

field_list fl_0 {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation hash0 {
   input {
      fl_0;
   }
   algorithm : random;
   output_width : 17;
}

field_list fl_1 {
    pkt.field_c_32;
    pkt.field_d_32;
    pkt.field_e_16;
}

field_list_calculation hash1 {
   input {
      fl_1;
   }
   algorithm : random;
   output_width : 17;
}

action action_0(){
    alu_0.execute_stateful_alu_from_hash(hash0);
}

action action_1(){
    alu_1.execute_stateful_alu_from_hash(hash1);
}

table table_0 {
    reads {
        pkt.field_a_32 : lpm;
    }
    
    actions {
        action_0;
    }
    size : 512;
}

table table_1 {
    reads {
        pkt.field_b_32 : lpm;
    }
    
    actions {
        action_1;
    }
    size : 512;
}

control ingress {
    apply(table_0);
    apply(table_1);
}
