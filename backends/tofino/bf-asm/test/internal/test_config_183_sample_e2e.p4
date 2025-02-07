#include "tofino/intrinsic_metadata.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type meta_t {
     fields {
         a : 16;
         b : 16;
         c : 16;
         d : 16;
     }
}


header ethernet_t ethernet;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action action_0(){
    sample_e2e(1, 7);
}

action action_1(){
}


table table_0 {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        action_0;
        action_1;
    }
    size : 1024;
}

control ingress { }

control egress {
    apply(table_0);
}
