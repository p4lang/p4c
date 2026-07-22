#include <core.p4>


@command_line("--loopsUnroll") header h1 {
    bit<8> a;
}

header h2 {
    bit<16> b;
}

header_union HU {
    h1 x;
    h2 y;
}

struct headers_t {
    HU[4] hus;
}

struct meta_t {
    bit<8> idx;
}

parser P(packet_in pkt, out headers_t hdrs, inout meta_t m);
package top(P p);
parser myp(packet_in pkt, out headers_t hdrs, inout meta_t m) {
    state start {
        pkt.extract<h1>(hdrs.hus[m.idx].x);
        transition accept;
    }
}

top(myp()) main;
