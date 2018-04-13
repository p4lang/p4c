#include <core.p4>

header H {
    bit<32> f;
}

parser p(packet_in pk) {
    H h;
    @name("p.vs") value_set<tuple<bit<32>, bit<2>>>(4) vs;
    state start {
        pk.extract<H>(h);
        transition select(h.f, 2w2) {
            vs: next;
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

