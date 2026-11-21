#include <core.p4>

parser simple(out bit<2> z);
package m(simple n);
parser p2_0(out bit<2> z2) {
    @name("p2_0.x1") bit<2> x1_0;
    @name("p2_0.x2") bit<2> x2_0;
    @name("p2_0.x3") bit<2> x3_0;
    state start {
        x1_0 = 2w0;
        x2_0 = 2w1;
        x3_0 = 2w2;
        z2 = 2w3 | x1_0 | x2_0 | x3_0;
        transition accept;
    }
}

m(p2_0()) main;
