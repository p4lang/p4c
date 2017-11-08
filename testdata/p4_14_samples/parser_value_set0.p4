
// two consecutive parser value sets

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;
header ethernet_t inner_ethernet;

parser_value_set pvs0;
parser_value_set pvs1;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        pvs0 : ingress;
        pvs1 : parse_inner_ethernet;
        default : ingress;
    }
}

parser parse_inner_ethernet {
    extract(inner_ethernet);
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
