#include <core.p4>

parser simple(out bit<1> z);
package m(simple n);
parser p_0(out bit<1> z) {
    bit<1> z1;
    state start {
        z1 = 1w0;
        z = z1;
        z = 1w0;
        transition accept;
    }
}

m(p_0()) main;

