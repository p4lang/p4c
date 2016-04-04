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

action count_c1_2() {
    count(c1, 2);
}

table t1 {
    actions {
        count_c1_1;
    }
}

table t2 {
    actions {
        count_c1_2;
    }
}

control ingress {
    apply(t1);
    apply(t2);
}

control egress {
}
