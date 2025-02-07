#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 12;
       field_b : 4;
       field_c : 16;
       field_d : 32;
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


action action_0(my_param_0){
    modify_field(pkt.field_c, my_param_0);
}

@pragma no_versioning 1
table table_0 {
   reads {
      pkt.field_a : ternary;
      pkt.field_d : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
