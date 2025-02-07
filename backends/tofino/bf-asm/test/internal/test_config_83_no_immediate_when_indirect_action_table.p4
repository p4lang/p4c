#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 16;
       field_b : 16;
       field_c : 16;
       field_d : 16;
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

action action_2(my_param_0, my_param_1){
    modify_field(pkt.field_a, my_param_0);
    modify_field(pkt.field_d, my_param_1);
}

//@pragma immediate 0
@pragma action_entries 1024
table table_0 {
   reads {
      pkt.field_b : ternary;
   }
   actions {
      action_0;
      action_1;
      action_2;
   }
   size : 2048;
}

control ingress {
    apply(table_0);
}
