#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p2_0(out bit<2> w) {
    bit<2> w_1;
    state start {
        w_1 = 2w2;
        w = w_1;
        transition accept;
    }
}

m(p2_0()) main;

