header_type test_t {
    fields {
        f : 8;
    }
}

metadata test_t test;
header test_t test;

action simple() {
    test.f = 0;
}

table simple_tbl {
    actions { simple; }
}

parser start {
    return ingress;
}

control ingress {
    apply (simple_tbl);
}
