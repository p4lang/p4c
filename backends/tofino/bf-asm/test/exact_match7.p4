
header_type data_t {
    fields {
        x1 : 3;
        f1 : 7;
        x2 : 6;
        f2 : 32;
        x3 : 5;
        f3 : 20;
        x4 : 7;
        f4 : 32;
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
        data.f3 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
