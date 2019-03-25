#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p2_0(out bit<2> w) {
    state start {
        transition p1_0_start;
    }
    state p1_0_start {
        w = 2w2;
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

m(p2_0()) main;

