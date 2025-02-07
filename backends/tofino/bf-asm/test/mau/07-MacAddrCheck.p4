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

header_type ingress_metadata_t {
    fields {
        drop : 1;
    }
}

metadata ingress_metadata_t ing_metadata;

action nop() {
}

action ing_drop() {
    drop();
}

table bad_mac_drop {
    actions {
        ing_drop;
    }
}

control ingress {
    if (ethernet.srcAddr == ethernet.dstAddr) {
        apply(bad_mac_drop);
    }
}

control egress {
}
