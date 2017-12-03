#include <core.p4>

parser simple(out bit<1> z);
package m(simple n);
parser p1_0(out bit<1> z1) {
    state start {
        z1 = 1w0;
        transition accept;
    }
}

parser p_0(out bit<1> z) {
    @name("p1i") p1_0() p1i_0;
    state start {
        p1i_0.apply(z);
        z = z & 1w0 & 1w1;
        transition accept;
    }
}

m(p_0()) main;

