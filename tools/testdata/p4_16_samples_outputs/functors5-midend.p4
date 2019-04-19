#include <core.p4>

parser simple(out bit<2> w);
package m(simple n);
parser p2_0(out bit<2> w) {
    state start {
        w = 2w2;
        transition accept;
    }
}

m(p2_0()) main;

