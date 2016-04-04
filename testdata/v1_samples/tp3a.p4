
header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        b1 : 32;
        b2 : 32;
        b3 : 32;
        b4 : 32;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setf1(val) { modify_field(data.f1, val); }
action setb1(val) { modify_field(data.b1, val); }
action setb2(val) { modify_field(data.b2, val); }
action setb3(val) { modify_field(data.b3, val); }
action setb4(val) { modify_field(data.b4, val); }

table A1 {
    reads { data.f1 : ternary; }
    actions { setb1; noop; }
}
table A2 {
    reads { data.b1 : ternary; }
    actions { setb3; noop; }
}
table A3 {
    reads { data.b3 : ternary; }
    actions { setb1; noop; }
}

table B1 {
    reads { data.f2 : ternary; }
    actions { setb2; noop; }
}

table B2 {
    reads { data.b2 : ternary; }
    actions { setb4; noop; }
}

control ingress {
    if (data.b1 == 0) {
        apply(A1);
        apply(A2);
        apply(A3);
    }
    apply(B1);
    apply(B2);
}

table E1 {
    reads { data.f2 : ternary; }
    actions { setf1; noop; }
}

table EA {
    reads { data.f3 : ternary; }
    actions { setb1; noop; }
}

table EB {
    reads { data.f4 : ternary; }
    actions { setb1; noop; }
}

table E2 {
    reads { data.b1 : ternary; }
    actions { setb4; noop; }
}

control egress {
    apply(E1);
    if (data.f1 == 0) {
        apply(EA);
    } else {
        apply(EB); }
    apply(E2);
}
