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

action action_0(){
    drop();
}

action action_1(){
    drop();
}

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


control ingress {
    apply(table_0);
}

control egress {
    apply(table_1);
}
