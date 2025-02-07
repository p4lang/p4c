#include "tofino/intrinsic_metadata.p4"

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        b1 : 8;
        b2 : 8;
    }
}

header_type extra_t {
    fields {
        h : 16;
        b1 : 8;
        b2 : 8;
    }
}

header data_t   data;
header extra_t  extra[4];

parser start {
    extract(data);
    return extra;
}
parser extra {
    extract(extra[next]);
    return select(latest.b2) {
        0x80 mask 0x80: extra;
        default: ingress;
    }
}

action setb1(port, val) {
    modify_field(data.b1, val);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}
action noop() { }
action setb2(val) { modify_field(data.b2, val); }
action set0b1(val) { modify_field(extra[0].b1, val); }
action set1b1(val) { modify_field(extra[1].b1, val); }
action set2b2(val) { modify_field(extra[2].b2, val); }
action act1(val) { modify_field(extra[0].b1, val); }
action act2(val) { modify_field(extra[0].b1, val); }
action act3(val) { modify_field(extra[0].b1, val); }

table test1 {
    reads { data.f1 : exact; }
    actions {
        setb1;
        noop;
    }
}
table ex1 {
    reads { extra[0].h : exact; }
    actions {
        set0b1;
        act1;
        act2;
        act3;
        noop;
    }
}
table tbl1 {
    reads { data.f2 : exact; }
    actions { setb2; noop; } }
table tbl2 {
    reads { data.f2 : exact; }
    actions { set1b1; noop; } }
table tbl3 {
    reads { data.f2 : exact; }
    actions { set2b2; noop; } }

control ingress {
    apply(test1);
    apply(ex1) {
        act1 { apply(tbl1); }
        act2 { apply(tbl2); }
        act3 { apply(tbl3); }
    }
}
