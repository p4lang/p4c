#include <core.p4>

parser simple(out bit<2> z);
package m(simple n);
parser p2_0(out bit<2> z2) {
    bit<2> x1;
    bit<2> x2;
    bit<2> x3;
    bit<2> z1;
    bit<2> z1_3;
    bit<2> z1_4;
    state start {
        z1 = 2w0;
        x1 = z1;
        z1_3 = 2w1;
        x2 = z1_3;
        z1_4 = 2w2;
        x3 = z1_4;
        z2 = 2w3 | x1 | x2 | x3;
        transition accept;
    }
}

m(p2_0()) main;

