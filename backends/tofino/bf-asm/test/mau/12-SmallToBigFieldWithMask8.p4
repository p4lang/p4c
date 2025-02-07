#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
#else
#include "includes/tofino.p4"
#endif

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

header_type m1_t {
    fields {
        f1 : 16;
    }
}

metadata m1_t m1;

action a1(p1) {
    modify_field(m1.f1, p1);
}

action a2() {
    modify_field(ethernet.dstAddr, m1.f1, 0xfffF);
    //modify_field(ethernet.dstAddr, m1.f1);
}

table t1 {
    actions {
        a1; 
    }
}

table t2 {
    actions {
        a2;
    }
}

control ingress {
    apply(t1);
    apply(t2);
}

control egress {
}
