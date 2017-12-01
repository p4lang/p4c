#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p1_0(out bit<2> w) {
    state start {
        w = 2w2;
        transition accept;
    }
}

parser p2_0(out bit<2> w) {
    @name("x") p1_0() x_0;
    state start {
        x_0.apply(w);
        transition accept;
    }
}

m(p2_0()) main;

