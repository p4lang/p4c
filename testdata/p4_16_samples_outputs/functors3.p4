#include <core.p4>

parser p1(out bit<1> z1)(bit<1> b1) {
    state start {
        z1 = b1;
        transition accept;
    }
}

parser p(out bit<1> z)(bit<1> b, bit<1> c) {
    p1(b) p1i;
    state start {
        p1i.apply(z);
        z = z & b & c;
        transition accept;
    }
}

const bit<1> bv = 1w0;
parser simple(out bit<1> z);
package m(simple n);
m(p(bv, 1w1)) main;

