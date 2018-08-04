header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

action send(port) {
    modify_field(standard_metadata.egress_port, port);
}

action discard() {
    drop();
}

#define T(name)                       \
table name {                          \
    reads {                           \
        ethernet.dstAddr : exact;     \
    }                                 \
    actions {                         \
        send; discard;                \
    }                                 \
    size : 1024;                      \
}

T(a1)
T(b1)
T(c1)
T(c2)

control c {
    if (standard_metadata.ingress_port & 0x02 == 1) {
        apply(c1);
    }
    if (standard_metadata.ingress_port & 0x04 == 1) { 
        apply(c2);
    }
}

control ingress {
    if (standard_metadata.ingress_port & 0x01 == 1) {
        apply(a1);
        c();
    } else {
        apply(b1);
        c();
    }
}

control egress {
}

