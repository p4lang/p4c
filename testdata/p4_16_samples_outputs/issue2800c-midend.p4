#include <core.p4>

header H {
    bit<16> a;
    int<16> b;
}

struct metadata {
}

struct Headers {
    H h;
}

parser p(packet_in packet, out Headers hdr) {
    state start {
        packet.extract<H>(hdr.h);
        transition select(hdr.h.a, (bit<16>)hdr.h.b) {
            (16w0 &&& 16w65528, 16w65531): parse;
            (16w0 &&& 16w65528, 16w65532 &&& 16w65532): parse;
            (16w0 &&& 16w65528, 16w0 &&& 16w65532): parse;
            (16w0 &&& 16w65528, 16w4): parse;
            default: accept;
        }
    }
    state parse {
        hdr.h.a = 16w1;
        hdr.h.b = -16s1;
        transition accept;
    }
}

parser Parser(packet_in b, out Headers hdr);
package top(Parser p);
top(p()) main;
