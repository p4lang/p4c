
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
header_type data2_t {
    fields {
        x1 : 8;
        x2 : 8;
    }
}
header data2_t data2;

parser start {
    extract(data);
    return select(data.b1) {
        0x01 : parse_data2;
        default : ingress;
    }
}

parser parse_data2 {
    extract(data2);
    return ingress;
}

action noop() { }
action setb1(val) { modify_field(data.b1, val); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setb1;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    if (valid(data2)) {
        apply(test1); }
    apply(test2);
}
