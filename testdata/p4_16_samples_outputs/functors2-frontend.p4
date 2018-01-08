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
        transition p1_0_start;
    }
    state p1_0_start {
        z1 = 2w0;
        transition start_0;
    }
    state start_0 {
        x1 = z1;
        transition p1_1_start;
    }
    state p1_1_start {
        z1_3 = 2w1;
        transition start_1;
    }
    state start_1 {
        x2 = z1_3;
        transition p1_2_start;
    }
    state p1_2_start {
        z1_4 = 2w2;
        transition start_2;
    }
    state start_2 {
        x3 = z1_4;
        z2 = 2w3 | x1 | x2 | x3;
        transition accept;
    }
}

m(p2_0()) main;

