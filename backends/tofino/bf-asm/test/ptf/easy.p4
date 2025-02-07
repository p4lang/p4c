#include "tofino/intrinsic_metadata.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action nop() { }

action do() { modify_field(ig_intr_md_for_tm.ucast_egress_port, 2); }

table t {
    reads { ig_intr_md.ingress_port : exact; }
    actions { nop; do; }
    default_action : do();
}

control ingress { apply(t); }
