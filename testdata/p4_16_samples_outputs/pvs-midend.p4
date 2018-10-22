#include <core.p4>

header H {
    bit<32> f;
}

parser p(packet_in pk) {
    H h_0;
    @name("p.vs") value_set<tuple<bit<32>, bit<2>>>(4) vs_0;
    state start {
        pk.extract<H>(h_0);
        transition select(h_0.f, 2w2) {
            vs_0: next;
            default: reject;
        }
    }
    state next {
        transition accept;
    }
}

parser ps(packet_in p);
package top(ps _p);
top(p()) main;

