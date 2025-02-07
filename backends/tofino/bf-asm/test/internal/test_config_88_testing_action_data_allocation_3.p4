#include "tofino/intrinsic_metadata.p4"

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
        meta_a_32 : 32;
        meta_b_32 : 32;
        meta_c_32 : 32;
        meta_d_32 : 32;
    }
}


header pkt_t pkt;
metadata meta_t meta;

parser start {
    return parse_test;
}

parser parse_test {
    extract(pkt);
    return ingress;
}


action nop(){
   no_op();
}

//Simple Actions

action action_0(param_a_32){
    modify_field(pkt.field_a_32, param_a_32);
}

action action_1(param_e_16){
    modify_field(pkt.field_e_16, param_e_16);
}

action action_2(param_i_8){
    modify_field(pkt.field_i_8, param_i_8);
}

//Adjacent Actions

action action_3(param_b_32){
    modify_field(pkt.field_b_32, param_b_32, 0xffffafff);
}

action action_4(param_f_16){
    modify_field(pkt.field_f_16, param_f_16, 0xfaff);
}

action action_5(param_j_8){
    modify_field(pkt.field_j_8, param_j_8, 0xaa);
}


//Long Actions


action action_6(param_c_32, param_g_16, param_k_8){
    modify_field(pkt.field_c_32, param_c_32);
    modify_field(pkt.field_g_16, param_g_16);
    modify_field(pkt.field_k_8, param_k_8);
}

action action_7(param_c_32, param_d_32, param_h_16, param_l_8, param_a_32, param_e_16){
    modify_field(pkt.field_c_32, param_c_32);
    modify_field(pkt.field_d_32, param_d_32);
    modify_field(pkt.field_h_16, param_h_16);
    modify_field(pkt.field_l_8, param_l_8);
    modify_field(pkt.field_a_32, param_a_32, 0xffcf);
    modify_field(pkt.field_e_16, param_e_16, 0xfcff);
}

action action_8(param_i_8, param_j_8, param_k_8){
    modify_field(pkt.field_i_8, param_i_8);
    modify_field(pkt.field_j_8, param_j_8);
    modify_field(pkt.field_k_8, param_k_8);
}

action action_9(param_e_16, param_f_16, param_g_16){
    modify_field(pkt.field_e_16, param_e_16);
    modify_field(pkt.field_f_16, param_f_16);
    modify_field(pkt.field_g_16, param_g_16);
}

action action_10(param_a_32, param_b_32, param_c_32, param_d_32){
    modify_field(pkt.field_a_32, param_a_32);
    modify_field(pkt.field_b_32, param_b_32);
    modify_field(pkt.field_c_32, param_c_32);
    modify_field(pkt.field_d_32, param_d_32);
}

action action_11(param_a_32, param_b_32, param_c_32, param_e_16, param_f_16,
    param_g_16, param_i_8, param_j_8, param_k_8, param_l_8){
    modify_field(pkt.field_a_32, param_a_32);
    modify_field(pkt.field_b_32, param_b_32);
    modify_field(pkt.field_c_32, param_c_32);
    modify_field(pkt.field_e_16, param_e_16);
    modify_field(pkt.field_f_16, param_f_16);
    modify_field(pkt.field_g_16, param_g_16);
    modify_field(pkt.field_i_8, param_i_8);
    modify_field(pkt.field_j_8, param_j_8);
    modify_field(pkt.field_k_8, param_k_8);
    modify_field(pkt.field_l_8, param_l_8);
}

action action_12(param_a_32, param_b_32, param_c_32, param_i_8, param_j_8, param_k_8, param_l_8){
    modify_field(pkt.field_a_32, param_a_32, 0xffcf);
    modify_field(pkt.field_b_32, param_b_32, 0xfcff);  
    modify_field(pkt.field_c_32, param_c_32);  
    modify_field(pkt.field_i_8, param_i_8);
    modify_field(pkt.field_j_8, param_j_8);
    modify_field(pkt.field_k_8, param_k_8);
    modify_field(pkt.field_l_8, param_l_8);
}

action action_13(param_a_32, param_b_32, param_c_32, param_e_16, param_f_16, param_g_16, param_h_16){
    modify_field(pkt.field_a_32, param_a_32, 0xffcf);
    modify_field(pkt.field_b_32, param_b_32, 0xfcff);  

    modify_field(pkt.field_c_32, param_c_32);

    modify_field(pkt.field_e_16, param_e_16);
    modify_field(pkt.field_f_16, param_f_16);
    modify_field(pkt.field_g_16, param_g_16);  
    modify_field(pkt.field_h_16, param_h_16);  
}

action action_14(param_a_32, param_b_32, param_c_32, param_d_32,
    param_meta_a_32){ //, param_meta_b_32, param_meta_c_32){
    modify_field(pkt.field_a_32, param_a_32, 0xffcf);
    modify_field(pkt.field_b_32, param_b_32, 0xfcff);  
    modify_field(pkt.field_c_32, param_c_32);
    modify_field(pkt.field_d_32, param_d_32);
    modify_field(meta.meta_a_32, param_meta_a_32);
}

action action_15(){
}

//@pragma immediate 0
@pragma action_entries 512
table table_0 {
   reads {
      pkt.field_b_32 : exact;
   }
   actions {
      action_0;
      action_1;
      action_2;
      action_3;
      action_4;
      action_5;
      action_6;
      action_7;
      action_8;
      action_9;
      action_10;
      action_11;
      action_12;
      action_13;
      action_14;
      action_15;
   }
   size : 1024;
}

control ingress {
    apply(table_0);
}
