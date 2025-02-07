#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
       field_b : 32;
       field_c : 32;
       field_d : 32;

       field_e : 16;
       field_f : 16;
       field_g : 16;
       field_h : 16;

       field_i : 8;
       field_j : 8;
       field_k : 8;
       field_l : 8;
   }
}

header pkt_t pkt;

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

action action_0(my_only_param){
    modify_field(pkt.field_a, my_only_param);
    modify_field(pkt.field_e, my_only_param);
    modify_field(pkt.field_i, my_only_param);
    
}


@pragma immediate 0
table table_0 {
   reads {
      pkt.field_b : exact;
   }
   actions {
      action_0;
   }
   size : 1024;
}

control ingress {
    apply(table_0);
}
