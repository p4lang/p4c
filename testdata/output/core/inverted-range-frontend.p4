#include <core.p4>

enum bit<16> V {
    z = 16w0,
    o = 16w1
}

parser p(in bit<16> x) {
    state start {
        transition select(x) {
            16w10 .. 16w0: accept;
            V.z .. V.o: accept;
            default: reject;
        }
    }
}

parser P(in bit<16> x);
package top(P _p);
top(p()) main;
