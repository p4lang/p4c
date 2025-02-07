#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
       field_b : 32;
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


action action_0(my_param_0, my_param_1){
    modify_field(pkt.field_a, my_param_0);
    modify_field(pkt.field_b, my_param_1);
}


table table_0 {
   reads {
      pkt.field_a : exact;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
