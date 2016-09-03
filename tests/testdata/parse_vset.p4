// Standard L2 Ethernet header
header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header ethernet_t ethernet;

header_type my_header_t {
    fields {
        f1 : 3;
        f2 : 13;
    }
}

header my_header_t my_header;

parser_value_set pv1;
parser_value_set pv2;

header_type my_meta_t {
    fields {
        pv_parsed : 8;
    }
}

metadata my_meta_t my_meta;

parser start {
    extract(ethernet);
    return select(ethernet.ethertype) {
        0x0800 : ingress;
        pv1 : parse_pv1;
        pv2 : parse_pv2;
        default : ingress;
    }
}

parser parse_pv1 {
    set_metadata(my_meta.pv_parsed, 1);
    extract(my_header);
    return select(my_header.f1, my_header.f2) {
        pv2 : parse_pv2;
        default : ingress;
    }
}

parser parse_pv2 {
    set_metadata(my_meta.pv_parsed, 2);
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
