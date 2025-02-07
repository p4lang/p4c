#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
       field_b : 32;
       field_c : 32;
       field_d : 32;
       field_e : 32;
       field_f : 4;
       field_g : 12;
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
      pkt.field_b : ternary;
      pkt.field_c : ternary;
      pkt.field_d : ternary;
      pkt.field_e : ternary;
      pkt.field_g : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
