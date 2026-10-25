#include <core.p4>

struct S {
    bit<16> f1;
    bit<8>  f2;
}

parser p(packet_in p, out S s) {
    @name("p.local") bit<16> local_0;
    @name("p.local") bit<8> local_1;
    state start {
        local_0 = p.lookahead<bit<16>>();
        s.f1 = local_0;
        local_1 = p.lookahead<bit<8>>();
        s.f2 = local_1;
        transition accept;
    }
}

parser simple(packet_in packet, out S s);
package top(simple e);
top(p()) main;
