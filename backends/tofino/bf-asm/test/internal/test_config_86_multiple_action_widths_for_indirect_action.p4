#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 8;
       field_b : 8;
       field_c : 16;
       field_d : 16;

       field_e : 32;
       field_f : 32;
       field_g : 32;
       field_h : 32;
       field_i : 32;
       field_j : 32;
       field_k : 32;
       field_l : 32;
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

//8
action action_0(my_param_a){
    modify_field(pkt.field_a, my_param_a);
}

//16
action action_1(my_param_c){
    modify_field(pkt.field_c, my_param_c);
}

//32
action action_2(my_param_e){
    modify_field(pkt.field_e, my_param_e);
}

//64
action action_3(my_param_f, my_param_g){
    modify_field(pkt.field_f, my_param_f);
    modify_field(pkt.field_g, my_param_g);
}

//128
action action_4(my_param_h, my_param_i, my_param_j, my_param_k){
    modify_field(pkt.field_h, my_param_h);
    modify_field(pkt.field_i, my_param_i);
    modify_field(pkt.field_j, my_param_j);
    modify_field(pkt.field_k, my_param_k);
}

//160
action action_5(my_param_h, my_param_i, my_param_j, my_param_k, my_param_l){
    modify_field(pkt.field_h, my_param_h);
    modify_field(pkt.field_i, my_param_i);
    modify_field(pkt.field_j, my_param_j);
    modify_field(pkt.field_k, my_param_k);
    modify_field(pkt.field_l, my_param_l);
}


@pragma immediate 0
@pragma action_entries 1024
table table_0 {
   reads {
      pkt.field_b : exact;
   }
   actions {
      action_0;
      action_1;
      action_2;
      action_3;
      action_4;
      action_5;
   }
   size : 4096;
}

control ingress {
    apply(table_0);
}
