#include <core.p4>

header H {
    bit<32> f;
}

parser p(packet_in pk) {
    H h_0;
    state start {
        pk.extract<H>(h_0);
        transition select(h_0.f) {
            32w0: next;
            default: accept;
        }
    }
    state next {
        transition accept;
    }
}

parser ps(packet_in p);
package top(ps _p);
top(p()) main;

