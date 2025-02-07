
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
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
action setb2(val) { modify_field(data.b2, val); }
action setb3(val) { modify_field(data.b3, val); }
action setb4(val) { modify_field(data.b4, val); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        noop;
        setb1;
        setb2;
        setb3;
        setb4;
    }
}

control ingress {
    if (data.f2 != 0) {
        apply(test1);
    }
}
