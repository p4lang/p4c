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
        transition select((bit<16>)hdr.h.a, hdr.h.b) {
            (16w65531, 16w0 &&& 16w65528): parse;
            (16w65532 &&& 16w65532, 16w0 &&& 16w65528): parse;
            (16w0 &&& 16w65532, 16w0 &&& 16w65528): parse;
            (16w4, 16w0 &&& 16w65528): parse;
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
