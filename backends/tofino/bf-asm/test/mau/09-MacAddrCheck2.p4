#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
#else
#include "includes/tofino.p4"
#endif

header_type ethernet_t {
    fields {
        dstAddr16   : 16;
        dstAddr32   : 32;
        srcAddr16   : 16;
        srcAddr32   : 32;
        ethertype   : 16;
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
    if (ethernet.srcAddr16 == ethernet.dstAddr16) { 
        if (ethernet.srcAddr32 == ethernet.dstAddr32) {
            apply(bad_mac_drop);
        }
    }
}

control egress {
}
