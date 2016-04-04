// Standard L2 Ethernet header
header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header ethernet_t ethernet;

// just ethernet
parser start {
    extract(ethernet);
    return ingress;
}

action route_eth(egress_spec, src_addr) {
    modify_field(standard_metadata.egress_spec, egress_spec);
    modify_field(ethernet.src_addr, src_addr);
}
action noop() { }

table routing {
    reads {
	ethernet.dst_addr : lpm;
    }
    actions {
	route_eth;
	noop;
    }
}

control ingress {
    apply(routing);
}
