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



/* ----------------------------------------- */
field_list f_em_direct {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_em_direct {
   input {
      f_em_direct;
   }
   algorithm : random;
   output_width : 12;
}

register r_em_direct {
    width : 16;
    static : t_em_direct;
    instance_count : 2048;
}

blackbox stateful_alu b_em_direct {
    reg: r_em_direct;
    update_lo_1_value: register_lo + 1;
    output_value : alu_lo;
    output_dst : pkt.field_e_16;
}

action a_em_direct(){
    b_em_direct.execute_stateful_alu_from_hash(h_em_direct);
}

table t_em_direct {
    reads {
        pkt.field_a_32 : exact;
    }
    
    actions {
        a_em_direct;
    }
    size : 4096;
}

/* ----------------------------------------- */
field_list f_em_indirect {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_em_indirect {
   input {
      f_em_indirect;
   }
   algorithm : random;
   output_width : 13;
}

register r_em_indirect {
    width : 16;
    instance_count: 8192;
}

blackbox stateful_alu b_em_indirect {
    reg: r_em_indirect;
    update_lo_1_value: register_lo + 5;
    output_value : alu_lo;
    output_dst : pkt.field_f_16;
}

action a_em_indirect(){
    b_em_indirect.execute_stateful_alu_from_hash(h_em_indirect);
}

table t_em_indirect {
    reads {
        pkt.field_a_32 : exact;
    }
    
    actions {
        a_em_indirect;
        do_nothing;
    }
    size : 2048;
}


/* ----------------------------------------- */
field_list f_t_direct {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_t_direct {
   input {
      f_t_direct;
   }
   algorithm : random;
   output_width : 12;
}

register r_t_direct {
    width : 16;
    static : t_t_direct;
    instance_count : 3072;
}

blackbox stateful_alu b_t_direct {
    reg: r_t_direct;
    update_lo_1_value: register_lo + 1;
    output_value : alu_lo;
    output_dst : pkt.field_g_16;
}

action a_t_direct(){
    b_t_direct.execute_stateful_alu_from_hash(h_t_direct);
}

table t_t_direct {
    reads {
        pkt.field_a_32 : ternary;
    }
    
    actions {
        a_t_direct;
    }
    size : 4096;
}


/* ----------------------------------------- */
field_list f_t_indirect {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_t_indirect {
   input {
      f_t_indirect;
   }
   algorithm : random;
   output_width : 13;
}

register r_t_indirect {
    width : 16;
    instance_count: 8192;
}

blackbox stateful_alu b_t_indirect {
    reg: r_t_indirect;
    update_lo_1_value: register_lo + 5;
    output_value : alu_lo;
    output_dst : pkt.field_h_16;
}

action a_t_indirect(){
    b_t_indirect.execute_stateful_alu_from_hash(h_t_indirect);
}

table t_t_indirect {
    reads {
        pkt.field_a_32 : ternary;
    }
    
    actions {
        a_t_indirect;
        do_nothing;
    }
    size : 2048;
}


/* ----------------------------------------- */
field_list f_no_key {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_no_key {
   input {
      f_no_key;
   }
   algorithm : random;
   output_width : 16;
}

register r_no_key {
    width : 16;
    static: t_no_key;
    instance_count : 1024;
}

blackbox stateful_alu b_no_key {
    reg: r_no_key;
    update_lo_1_value: register_lo + 5;
    output_value : alu_lo;
    output_dst : pkt.field_i_8;
}

action a_no_key(){
    b_no_key.execute_stateful_alu_from_hash(h_no_key);
}

table t_no_key {
    actions {
        a_no_key;
    }
    size : 1024;
}


/* ----------------------------------------- */
field_list f_hash_act {
    pkt.field_a_32;
    pkt.field_b_32;
}

field_list_calculation h_hash_act {
   input {
      f_hash_act;
   }
   algorithm : random;
   output_width : 10;
}

register r_hash_act {
    width : 8;
    static: t_hash_act;
    instance_count : 256;
}

blackbox stateful_alu b_hash_act {
    reg: r_hash_act;
    update_lo_1_value: register_lo + 5;
    output_value : alu_lo;
    output_dst : pkt.field_j_8;
}

action a_hash_act(){
    b_hash_act.execute_stateful_alu_from_hash(h_hash_act);
}

table t_hash_act {
    reads {
        pkt.field_d_32 mask 0x3ff : exact;
    }
    actions {
        a_hash_act;
    }
    size : 1024;
}

action do_nothing(){}

/* Main control flow */

control ingress {
    apply(t_em_direct);
    apply(t_em_indirect);
    apply(t_t_direct);
    apply(t_t_indirect);
    apply(t_no_key);
    apply(t_hash_act);
}
