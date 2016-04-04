
header_type data_t {
    fields {
        f1 : 128;
        f2 : 128;
        f3 : 128;
        f4 : 128;
        b1 : 8;
        b2 : 8;
        b3 : 8;
        b4 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setb1(val) { modify_field(data.b1, val); }

table test1 {
    reads {
        data.f1 : exact;
        data.f2 : exact;
        data.f3 : exact;
    }
    actions {
        setb1;
        noop;
    }
    size: 10000;
}

control ingress {
    apply(test1);
}
