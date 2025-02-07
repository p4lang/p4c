#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        0x800 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        drop: 1;
    }
}

metadata routing_metadata_t /*metadata*/ routing_metadata;


action nop(){
    no_op();
}

action mod_mac_adr(dstmac) {
    modify_field(ethernet.dstAddr, dstmac);
}


table table_0 {
    reads {
        ipv4.hdrChecksum : exact;
    }
    actions {
      nop;
      mod_mac_adr;
    }
}


control ingress{
    apply(table_0);
}
