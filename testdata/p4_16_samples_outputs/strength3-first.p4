#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<16> a;
    bit<16> b;
    bit<8>  c;
}

struct Headers {
    hdr h;
}

struct Meta {
    bit<8> x;
    bit<8> y;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action case0() {
        h.h.c = (bit<8>)((16w0 ++ h.h.a)[15:0] ++ 16w0[15:0]);
    }
    action case1() {
        h.h.c = (bit<8>)h.h.a[15:0];
    }
    action case2() {
        h.h.c = (bit<8>)16w0[15:0];
    }
    action case3() {
        h.h.c = h.h.a[7:0];
    }
    action case4() {
        h.h.c = (bit<8>)(16w0[7:0] ++ h.h.a[15:0]);
    }
    action case5() {
        h.h.c = (bit<8>)(16w0[7:0] ++ h.h.a[15:8]);
    }
    action case6() {
        h.h.c = (bit<8>)(16w0[15:0] ++ h.h.a[15:8]);
    }
    action case7() {
        h.h.c = (bit<8>)(16w0 ++ h.h.a >> 3)[31:8];
    }
    table t {
        actions = {
            case0();
            case1();
            case2();
            case3();
            case4();
            case5();
            case6();
            case7();
        }
        const default_action = case0();
    }
    apply {
        t.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<hdr>(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<hdr>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

