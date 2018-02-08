
// two consecutive parser value sets separate by a normal parse state

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

header ethernet_t ethernet;
header ethernet_t inner_ethernet;
header ipv4_t ipv4;

@pragma parser_value_set_size 4
parser_value_set pvs0;
@pragma parser_value_set_size 10
parser_value_set pvs1;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType, latest.srcAddr) {
        0x800 : parse_inner_ethernet;
        pvs0 : parse_inner_ethernet;
        default : ingress;
    }
}

parser parse_inner_ethernet {
    extract(inner_ethernet);
    return ingress;
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

table dummy {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        noop;
    }
    size : 512;
}

action noop() {
    no_op();
}

control ingress {
    apply(dummy);
}
