#include <core.p4>

header h_t {
    bit<8> a;
}

struct headers_t {
    h_t h;
}

struct tuple_0 {
    bit<8> f0;
}

parser P(packet_in pkt, out headers_t hdr, in tuple_0 m);
struct tuple_1 {
    bit<8> f0;
    bit<8> f1;
}

control C(inout headers_t hdr, in tuple_1 m);
struct tuple_2 {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
}

package top(P p, C c, tuple_2 cfg);
parser p(packet_in pkt, out headers_t hdr, in tuple_0 m) {
    state start {
        pkt.extract<h_t>(hdr.h);
        transition select(m.f0) {
            8w0: accept;
            default: reject;
        }
    }
}

control c(inout headers_t hdr, in tuple_1 m) {
    @hidden action tuplearchtypes35() {
        hdr.h.a = m.f0 + m.f1;
    }
    @hidden table tbl_tuplearchtypes35 {
        actions = {
            tuplearchtypes35();
        }
        const default_action = tuplearchtypes35();
    }
    apply {
        tbl_tuplearchtypes35.apply();
    }
}

top(p(), c(), { 8w1, 8w2, 8w3 }) main;
