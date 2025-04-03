#include <core.p4>

struct S {
    bit<16> f1;
    bit<8> f2;
}

parser p(packet_in p, out S s) {
    state start { transition s0; }
    state s0 {
        bit<16> local = p.lookahead<bit<16>>();
        s.f1 = local;
        transition s1;
    }
    state s1 {
        bit<8> local = p.lookahead<bit<8>>();
        s.f2 = local;
        transition s2;
    }
    state s2 { transition accept; }
}

parser simple(packet_in packet, out S s);
package top(simple e);
top(p()) main;

