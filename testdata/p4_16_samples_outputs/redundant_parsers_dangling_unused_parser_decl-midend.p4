#include <core.p4>

header h_t {
    bit<16> f;
}

struct headers {
    h_t h1;
    h_t h2;
}

parser IParser(packet_in packet, inout headers hdr) {
    state start {
        packet.extract<h_t>(hdr.h1);
        transition select(hdr.h1.f) {
            16w1: s1;
            default: s2;
        }
    }
    state s1 {
        transition accept;
    }
    state s2 {
        packet.extract<h_t>(hdr.h2);
        transition accept;
    }
}

parser Parser(packet_in p, inout headers hdr);
package top(Parser p);
top(IParser()) main;
