header_type h {
    fields {
        b : 1;
    }
}

metadata h m;

parser start {
    return ingress;
}

action x() {}

table t {
    actions { x; }
}

control c {
    if (m.b == 1) { apply(t); }
}

control d {
    c();
}

control ingress {
    d();
}