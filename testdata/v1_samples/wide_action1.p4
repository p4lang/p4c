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

header_type metadata_t {
    fields {
        m0 : 32;
        m1 : 32;
        m2 : 32;
        m3 : 32;
        m4 : 32;
        m5 : 16;
        m6 : 16;
        m7 : 16;
        m8 : 16;
        m9 : 16;
        m10 : 8;
        m11 : 8;
        m12 : 8;
        m13 : 8;
        m14 : 8;
    }
}
metadata metadata_t m;

parser start {
    extract(data);
    return ingress;
}

action setmeta(v0, v1, v2, v3, v4, v5, v6) {
    modify_field(m.m0, v0);
    modify_field(m.m1, v1);
    modify_field(m.m2, v2);
    modify_field(m.m3, v3);
    modify_field(m.m4, v4);
    modify_field(m.m5, v5);
    modify_field(m.m6, v6);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setmeta;
    }
}

control ingress {
    apply(test1);
}
