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


action action_15(){
}

table table_0 {
   reads {
      pkt.field_a_32 : exact;
      pkt.field_b_32 : exact;
      pkt.field_c_32 : exact;
      pkt.field_e_16 : exact;
   }
   actions {
      action_0;
      action_15;
   }
   size : 200000;
}

control ingress {
    if (valid(pkt)){
        apply(table_0);
    }
}
