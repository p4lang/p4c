header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

header_type h_t {
    fields {
        f1 : 13;
    }
}

header h_t h;

parser start {
    extract(ethernet);
    return ingress;
}

action nop() {
}

table t1 {
    actions {
        nop;
    }
}

control ingress {
    if (h.f1 > 1) {
        apply(t1);
    }
}

control egress {
}
