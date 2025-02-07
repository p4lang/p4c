#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
       field_b : 32;
       field_c : 32;
       field_d : 32;
       field_e : 32;
       field_f : 32;
       field_g : 16;
       field_h : 12;
       field_i : 4;
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

action action_2(my_param_0, my_param_1, my_param_2, my_param_3){
    modify_field(pkt.field_a, my_param_0);
    modify_field(pkt.field_b, my_param_1);
    modify_field(pkt.field_c, my_param_2);
    modify_field(pkt.field_d, my_param_3);
}


@pragma action_entries 100
table table_0 {
   reads {
      pkt.field_a : ternary;
   }
   actions {
      action_0;
      action_1;
      action_2;
   }
   min_size : 2049;
}

control ingress {
    apply(table_0);
}
