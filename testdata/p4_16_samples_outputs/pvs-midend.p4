#include <core.p4>

header H {
    bit<32> f;
}

parser p(packet_in pk) {
    @name("p.vs") value_set<tuple<bit<32>, bit<2>>> vs;
    H h;
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

