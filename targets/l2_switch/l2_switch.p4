header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type intrinsic_metadata_t {
    fields {
        learn_id : 4;
        mgid : 4;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

#define MAC_LEARN_RECEIVER 1

field_list mac_learn_digest {
    ethernet.srcAddr;
    standard_metadata.ingress_port;
}

action mac_learn() {
    // hack to force BM to enable arithmetic on this field
    modify_field(intrinsic_metadata.learn_id, 0);
    generate_digest(MAC_LEARN_RECEIVER, mac_learn_digest);
}

table smac {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {mac_learn; _nop;}
    size : 512;
}

action forward(port) {
    modify_field(standard_metadata.egress_port, port);
}

action broadcast() {
    modify_field(intrinsic_metadata.mgid, 1);
}

table dmac {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {forward; broadcast;}
    size : 512;
}

control ingress{
    apply(smac);
    apply(dmac);
}
