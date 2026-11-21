#include <core.p4>

extern bool ext();
parser P(in bit<6> ttt);
package Pkg(P p);
parser MyParser1(in bit<6> ttt) {
    state start {
        transition select(ext()) {
            true: s1;
            false: s2;
        }
    }
    state s1 {
        transition ttt;
    }
    state s2 {
        transition accept;
    }
}

Pkg(MyParser1()) main;
