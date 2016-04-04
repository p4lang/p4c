parser start {
    return ingress;
}

header_type m_t {
    fields {
        f1 : 32;
    }
}

metadata m_t m;

action a1() {
    modify_field(m.f1, 1);
}

table t1 {
    actions {
        a1;
    }
}

control ingress {
    apply(t1);
}

control egress {
}
