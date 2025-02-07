#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
       field_b : 32;
       field_c : 32;
       field_d : 32;

       field_e : 32;
       field_f : 32;
       field_g : 32;
       field_h : 32;
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


action action_0(){
    no_op();
}

action action_1(my_param_0){
    modify_field(pkt.field_c, my_param_0);
}

table table_0 {
   reads {
      pkt.field_a : exact;
      pkt.field_b : exact;
      pkt.field_c : exact;
      pkt.field_d : exact;
      pkt.field_e : exact;
   }
   actions {
      action_0;
      action_1;
   }
}

control ingress {
    apply(table_0);
}
