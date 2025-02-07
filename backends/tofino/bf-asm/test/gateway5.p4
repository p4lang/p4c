header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        x1 : 2;
        pad0 : 3;
        x2 : 2;
        pad1 : 5;
        x3 : 1;
        pad2 : 2;
        skip : 32;
        x4 : 1;
        x5 : 1;
        pad3 : 6;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setf4(val) { modify_field(data.f4, val); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setf4;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setf4;
        noop;
    }
}

control ingress {
    if (data.x1 == 1 and data.x4 == 0) {
        apply(test1);
    } else {
        apply(test2);
    }
}
