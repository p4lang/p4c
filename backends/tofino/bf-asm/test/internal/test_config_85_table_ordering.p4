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
       field_i : 32;
       field_j : 32;
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

action action_0(my_param_0, my_param_4){
    modify_field(pkt.field_a, my_param_0);
    modify_field(pkt.field_g, my_param_4);
}

action action_1(my_param_1){
    modify_field(pkt.field_c, my_param_1);
}

action action_2(my_param_2){
    modify_field(pkt.field_e, my_param_2);
}

action action_3(my_param_3){
    modify_field(pkt.field_i, my_param_3);
}

action action_4(my_param_4){
    modify_field(pkt.field_j, my_param_4);
}

@pragma immediate 0
table table_0 {
   reads {
      pkt.field_b : exact;
   }
   actions {
      action_0;
      nop;
   }
   size : 256000;
}

table table_1 {
   reads {
      pkt.field_d : exact;
   }
   actions {
      action_1;
      nop;
   }
}

table table_2 {
   reads {
      pkt.field_f : exact;
   }
   actions {
      action_2;
      nop;
   }
}

table table_3 {
   reads {
      pkt.field_h : exact;
   }
   actions {
      action_3;
      nop;
   }
}


table table_4 {
   reads {
      pkt.field_e : exact;
   }
   actions {
      action_4;
      nop;
   }
}



control ingress {
    apply(table_0){
       nop { 
           apply(table_1);
       }
    }

    if (valid(pkt)){
        apply(table_2);
    } else {
        apply(table_3);
    }

    apply(table_4);

}
