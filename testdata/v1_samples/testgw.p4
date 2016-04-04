// Standard L2 Ethernet header
header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header ethernet_t ethernet;

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 16;
        f4 : 16;
        f5 : 8;
        f6 : 8;
        f7 : 4;
        f8 : 4;
    }
}
header data_t data;

parser start {
    extract(ethernet);
    return data;
}
parser data {
    extract(data);
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

action setf2(val) {
    modify_field(data.f2, val);
    //add_to_field(data.f3, data.f4);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setf2;
        noop;
    }
}

action setf1(val) {
    modify_field(data.f1, val);
    //add_to_field(data.f4, 1);
}

table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setf1;
        noop;
    }
}

control ingress {
    apply(routing);
    if (data.f5 != data.f6) {
        apply(test1);
    } else {
        apply(test2);
    }
}

control egress {
}
