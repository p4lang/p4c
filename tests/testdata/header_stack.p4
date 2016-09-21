header_type hdr_t {
    fields {
        f1 : 16;
        f2 : 16;
    }
}

header hdr_t hdr[3];

parser start {
    extract(hdr[0]);
    extract(hdr[1]);
    return ingress;
}

action a1(v1) {
    modify_field(hdr[1].f1, v1);
    pop(hdr, 1);
}

table t1 {
    actions { a1; }
}

control ingress {
    apply(t1);
}

control egress { }
