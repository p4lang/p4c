parser start {
    return ingress;
}

header_type m_t {
    fields {
        f1 : 32;
        f2 : 32;
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

action a2() {
    modify_field(m.f2, 2);
}

table t2 {
    reads {
        m.f1 : exact;
    }
    actions {
        a2;
    }
}

control ingress {
    apply(t1);
    apply(t2);
}

control egress {
}
