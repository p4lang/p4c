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

action ig_drop() {
    modify_field(routing_metadata.drop, 1);
}

action hop(ttl, egress_port) {
    add_to_field(ttl, -1);
    modify_field(standard_metadata.egress_port, egress_port);
}

action do_nothing(){
    no_op();
}

action l3_set_index(index) {
    modify_field( ipv4.diffserv, index);
}

action hop_ipv4(srcmac, srcip, dstmac, egress_port){
    hop(ipv4.ttl, egress_port);
    modify_field(ipv4.srcAddr, srcip);
    modify_field(ethernet.srcAddr, srcmac);
    modify_field(ethernet.dstAddr, dstmac);
}

table ipv4_routing {
    reads {
        ipv4.dstAddr : lpm;
        ipv4.srcAddr : exact;
    }
    actions {
      ig_drop;
      hop_ipv4;
    }
    max_size : 2048;
}

table host_ip {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
        do_nothing; 
        l3_set_index;
    }
    max_size : 16384;
}


/* Main control flow */
control ingress {
    apply(ipv4_routing);
    apply(host_ip);
}

action eg_drop() {
    modify_field(standard_metadata.egress_spec, 0);
}

action permit() {
}

table egress_acl {
    reads {
        routing_metadata.drop: ternary;
    }
    actions {
        eg_drop;
        permit;
    }
}

control egress {
    apply(egress_acl);
}
