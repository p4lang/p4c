#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p2_0(out bit<2> w) {
    bit<2> w_1;
    state start {
        transition p1_0_start;
    }
    state p1_0_start {
        w_1 = 2w2;
        transition start_0;
    }
    state start_0 {
        w = w_1;
        transition accept;
    }
}

m(p2_0()) main;

