header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

counter c1 {
    type : packets;
    instance_count : 1024;
}

action count_c1_1() {
    count(c1, 1);
}

table t1 {
    actions {
        count_c1_1;
    }
}

control ingress {
    apply(t1);
}

control egress {
}
