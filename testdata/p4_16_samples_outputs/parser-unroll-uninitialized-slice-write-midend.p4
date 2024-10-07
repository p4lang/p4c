#include <core.p4>

struct S {
    bit<8> f;
}

parser p(packet_in packet, out S s) {
    state start {
        s.f[3:0] = 4w2;
        transition accept;
    }
}

parser simple(packet_in packet, out S s);
package top(simple e);
top(p()) main;
