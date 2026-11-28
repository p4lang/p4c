#include <core.p4>

header h_t {
    bit<8> a;
}

struct headers_t {
    h_t h;
}

parser P(packet_in pkt, out headers_t hdr, in tuple<bit<8>> m);
control C(inout headers_t hdr, in tuple<bit<8>, bit<8>> m);
package top(P p, C c, tuple<bit<8>, bit<8>, bit<8>> cfg);
parser p(packet_in pkt, out headers_t hdr, in tuple<bit<8>> m) {
    state start {
        pkt.extract(hdr.h);
        transition select(m[0]) {
            0: accept;
            default: reject;
        }
    }
}

control c(inout headers_t hdr, in tuple<bit<8>, bit<8>> m) {
    apply {
        hdr.h.a = m[0] + m[1];
    }
}

top(p(), c(), { 8w1, 8w2, 8w3 }) main;
