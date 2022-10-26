#include <core.p4>

header H {
    int<16> a;
    bit<16> b;
}

struct metadata {
}

struct Headers {
    H h;
}

parser p(packet_in packet, out Headers hdr) {
    state start {
        packet.extract<H>(hdr.h);
        transition select(hdr.h.a, hdr.h.b) {
            (-16s5 .. 16s4, 16w0 .. 16w7): parse;
            default: accept;
        }
    }
    state parse {
        hdr.h.a = -16s1;
        hdr.h.b = 16w1;
        transition accept;
    }
}

parser Parser(packet_in b, out Headers hdr);
package top(Parser p);
top(p()) main;
