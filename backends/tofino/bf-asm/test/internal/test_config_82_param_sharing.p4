#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 4;
       field_b : 4;
       field_c : 4;
       field_d : 4;
       field_e : 4;
       field_f : 4;
       field_g : 4;
       field_h : 4;

       field_i : 4;
       field_j : 4;
       field_k : 4;
       field_l : 4;
       field_m : 4;
       field_n : 4;
       field_o : 4;
       field_p : 4;

       field_q : 32;
       field_r : 32;

       field_s : 16;
       field_t : 16;
       field_u : 16;
       field_v : 16;

       field_w : 8;
       field_x : 8;
       field_y : 8;
       field_z : 8;


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
    modify_field(pkt.field_i, my_param_1);
}

table table_0 {
   reads {
      pkt.field_b : ternary;
   }
   actions {
      action_0;
      action_1;
      action_2;
   }
}

control ingress {
    apply(table_0);
}
