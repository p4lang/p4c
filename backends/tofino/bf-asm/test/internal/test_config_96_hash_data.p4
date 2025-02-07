#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_28 : 28;
        field_a2_4 : 4;
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
    return parse_pkt;
}

header pkt_t pkt;

parser parse_pkt {
    extract(pkt);
    return ingress;
}




field_list field_list_0 {
      pkt.field_a_28;
      pkt.field_b_32;
      pkt.field_i_8;
}

field_list field_list_1 {
      pkt.field_c_32;
      pkt.field_g_16;
      pkt.field_h_16;
      pkt.field_k_8;
}

field_list field_list_2 {
      pkt.field_d_32;
      pkt.field_l_8;
      pkt.field_c_32;
      pkt.field_e_16;
      pkt.field_f_16;
}

field_list_calculation hash_0 {
   input {
       field_list_0;
   }
   algorithm : crc32;
   output_width : 32;
}

field_list_calculation hash_1 {
   input {
       field_list_1;
   }
   algorithm : crc16;
   output_width : 16;
}

field_list_calculation hash_2 {
   input {
       field_list_2;
   }
   algorithm : random;
   output_width : 72;
}

action action_0(param0){
    modify_field(pkt.field_e_16, param0);
    modify_field_with_hash_based_offset(pkt.field_a_28, 0, hash_0, 16384);
}

action action_1(){
    modify_field_with_hash_based_offset(pkt.field_l_8, 0, hash_1, 256);
}

action action_2(param0){
    modify_field(pkt.field_h_16, param0);
}


action do_nothing(){
    no_op();
}


@pragma include_idletime 1
@pragma idletime_precision 1
table table_0 {
    reads {
        pkt.field_a_28 : exact;
    }
    actions {
        action_0;
    }
}


@pragma ways 8
@pragma include_idletime 1
table table_1 {
    reads {
        pkt.field_c_32 : exact;
        pkt.field_d_32 : exact;

        pkt.field_f_16 : exact;
        pkt.field_g_16 : exact;
        pkt.field_h_16 : exact;
    }
    actions {
        action_1;
        do_nothing;
    }
    size : 2048;
}

@pragma selector_max_group_size 121
@pragma include_idletime 1
@pragma idletime_sweep_interval 12
table table_2 {
    reads {
       pkt.field_b_32 : ternary;//exact;
    }
    action_profile : table_2_action_profile;
    size : 2048;
}

action_profile table_2_action_profile {
    actions {
       action_2;
       do_nothing;
    }
    size : 512; 
    dynamic_action_selection : table_2_selector;
}

action_selector table_2_selector {
    selection_key : hash_2;
}



/* Main control flow */

control ingress {
    if (valid(pkt)){
        apply(table_0);
    }

    if (valid(pkt)){
      apply(table_1);
    }


    apply(table_2);
}
