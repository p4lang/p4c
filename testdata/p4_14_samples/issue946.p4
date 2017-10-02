header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

parser_value_set pvs;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        pvs : ingress;
        default : ingress;
    }
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
