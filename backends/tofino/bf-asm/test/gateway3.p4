
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
    if (data.b2 == data.b3 and data.b4 == 10) {
        if (data.b1 == data.b2) {
            apply(test1);
        }
    } else { if (data.b1 != data.b2) {
        apply(test2);
    } }
}
