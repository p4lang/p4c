#include <core.p4>

parser p(out bit<1> z) {
    bit<1> z1_0;
    state start {
        transition p1_0_start;
    }
    state p1_0_start {
        z1_0 = 1w0;
        transition start_0;
    }
    state start_0 {
        z = z1_0;
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;

