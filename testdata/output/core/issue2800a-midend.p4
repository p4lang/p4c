#include <core.p4>

header H {
    int<16> a;
}

struct metadata {
}

struct Headers {
    H h;
}

parser p(packet_in packet, out Headers hdr) {
    state start {
        packet.extract<H>(hdr.h);
        transition select((bit<16>)hdr.h.a) {
            16w65531: parse;
            16w65532 &&& 16w65532: parse;
            16w0 &&& 16w65532: parse;
            16w4: parse;
            default: accept;
        }
    }
    state parse {
        hdr.h.a = -16s1;
        transition accept;
    }
}

parser Parser(packet_in b, out Headers hdr);
package top(Parser p);
top(p()) main;
