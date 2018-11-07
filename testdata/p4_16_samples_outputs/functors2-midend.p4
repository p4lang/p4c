#include <core.p4>

parser simple(out bit<2> z);
package m(simple n);
parser p2_0(out bit<2> z2) {
    bit<2> x1_0;
    bit<2> x2_0;
    bit<2> x3_0;
    bit<2> z1_0;
    bit<2> z1_1;
    bit<2> z1_2;
    state start {
        z1_0 = 2w0;
        x1_0 = z1_0;
        z1_1 = 2w1;
        x2_0 = z1_1;
        z1_2 = 2w2;
        x3_0 = z1_2;
        z2 = 2w3 | x1_0 | x2_0 | x3_0;
        transition accept;
    }
}

m(p2_0()) main;

