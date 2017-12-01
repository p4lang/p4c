#include <core.p4>

parser p1_0(out bit<1> z1) {
    state start {
        z1 = 1w0;
        transition accept;
    }
}

parser p(out bit<1> z) {
    @name("p1i") p1_0() p1i_0;
    state start {
        p1i_0.apply(z);
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;

