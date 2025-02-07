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

header_type tags_t {
    fields {
       field_a_8 : 8;
       field_b_12 : 12;
       field_c_4 : 4;
       field_d_8 : 8;
    }
}



header pkt_t pkt;
header tags_t tags[5];

parser start {
    return parse_test;
}

parser parse_test {
    extract(pkt);
    extract(tags[0]);
    return ingress;
}


action nop(){
   no_op();
}

action push_action(){
    push(tags, 3);
}

action pop_action(){
    pop(tags, 4);
}
table table_0 {
   reads {
      pkt.field_a_32 : exact;
   }
   actions {
      push_action;
      pop_action;
   }
   size : 1024;
}

control ingress {
    apply(table_0);
}
