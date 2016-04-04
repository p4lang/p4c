
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        b1 : 8;
        b2 : 8;
        h1 : 16;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action setbyte(dest, val) {
    modify_field(dest, val);
}

action noop() { }
action setb1(val, port) {
    setbyte(data.b1, val);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
        data.f1 : ternary;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
