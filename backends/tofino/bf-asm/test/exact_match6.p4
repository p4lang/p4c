
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

header_type meta_t {
    fields {
        sum : 32;
    }
}
metadata meta_t meta;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action addf2() { add(meta.sum, data.f2, 100); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        addf2;
        noop;
    }
}

control ingress {
    apply(test1);
}
