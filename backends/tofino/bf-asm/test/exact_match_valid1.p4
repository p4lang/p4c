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
header data_t data2;

parser start {
    extract(data);
    return select(data.f2) {
    0xf0000000 mask 0xf0000000 : parse_data2;
    default : ingress;
    }
}
parser parse_data2 {
    extract(data2);
    return ingress;
}

action noop() { }
action setb1(val) { modify_field(data2.b1, val); }

table test1 {
    reads {
        data : valid;
        data2 : valid;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
