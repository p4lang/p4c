#include <core.p4>

parser p1(out bit<1> z1)(bit<1> b1) {
    state start {
        z1 = b1;
        transition accept;
    }
}

parser p(out bit<1> z) {
    p1(1w0) p1i;
    state start {
        p1i.apply(z);
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;

