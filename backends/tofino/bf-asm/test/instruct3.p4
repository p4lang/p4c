
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        x1 : 2;
        x2 : 2;
        x3 : 2;
        x4 : 2;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setx() { modify_field(data.x2, 2); modify_field(data.x4,1); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setx;
        noop;
    }
}

control ingress {
    apply(test1);
}
