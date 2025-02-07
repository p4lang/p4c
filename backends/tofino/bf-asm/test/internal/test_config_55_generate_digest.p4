#include "tofino/intrinsic_metadata.p4"

header_type pkt_t {
   fields {
       field_a : 32;
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


field_list tcp_digest {
    pkt.field_a;
}


action action_0(){
   generate_digest(0, tcp_digest);
}


table table_0 {
   reads {
      pkt.field_a : ternary;
   }
   actions {
      action_0;
   }
}

control ingress {
    apply(table_0);
}
