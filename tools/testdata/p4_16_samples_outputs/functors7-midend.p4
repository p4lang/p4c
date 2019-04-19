#include <core.p4>

parser simple(in bit<2> a);
package m(simple n);
parser p1_0(in bit<2> a) {
    state start {
        transition accept;
    }
}

m(p1_0()) main;

