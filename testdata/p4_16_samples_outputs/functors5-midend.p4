#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p2_0(out bit<2> w) {
    bit<2> w_0;
    state start {
        w_0 = 2w2;
        w = w_0;
        transition accept;
    }
}

m(p2_0()) main;

