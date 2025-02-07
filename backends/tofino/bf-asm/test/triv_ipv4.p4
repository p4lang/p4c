header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header_type ipv4_t {
    fields {
        version         : 4;
        ihl             : 4;
        diffserv        : 8;
        totalLen        : 16;
        identification  : 16;
        flags           : 3;
        fragOffset      : 13;
        ttl             : 8;
        protocol        : 8;
        hdrChecksum     : 16;
        srcAddr         : 32;
        dstAddr         : 32;
//        options         : *;  // Variable length options
    }
//    length : ihl * 4;
//    max_length : 60;
}

header ethernet_t ethernet;
header ipv4_t ipv4;

// Start with ethernet always.
parser start {
    return ethernet;
}

parser ethernet {
    extract(ethernet);   // Start with the ethernet header
    return select(ethernet.ethertype) {
        0x800: ipv4;
//        default: ingress;
    }
}

parser ipv4 {
    extract(ipv4);
    return ingress;
}

action route_ipv4(egress_spec) {
    add_to_field(ipv4.ttl, -1);
    modify_field(standard_metadata.egress_spec, egress_spec);
}
action do_drop() { /*drop();*/ }
action do_noop() { }

table routing {
    reads {
	ipv4.dstAddr : lpm;
    }
    actions {
	do_drop;
	route_ipv4;
    }
    size: 2048;
}

control ingress {
    apply(routing);
}
control egress {
}
