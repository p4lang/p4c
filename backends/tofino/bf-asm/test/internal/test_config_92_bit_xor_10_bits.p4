#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a_32 : 32;
       field_b_16 : 16;
   }
}

header_type meta_t {
   fields {
       field_a_10 : 10;
       field_b_10 : 10;
   } 
}

header pkt_t pkt;
metadata meta_t meta;

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
    bit_xor(meta.field_a_10, meta.field_a_10, meta.field_b_10);
}


action action_15(){
}

//@pragma pa_atomic ingress meta.field_a_10 meta.field_b_10
table table_0 {
   reads {
      pkt.field_a_32 : ternary;
   }
   actions {
      action_0;
      action_15;
   }
   size : 1024;
}

control ingress {
    apply(table_0);
}
