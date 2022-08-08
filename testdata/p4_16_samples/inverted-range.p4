#include <core.p4>

enum bit<16> V {
    z = 0,
    o = 1
}

parser p(in bit<16> x) {
    state start {
        transition select(x) {
            10 .. 0: accept;
            V.z .. V.o: accept;
            _      : reject;
        }
    }
}

parser P(in bit<16> x);
package top(P _p);

top(p()) main;
