#include <core.p4>

parser simple(out bit<1> z);
package m(simple n);
parser p_0(out bit<1> z) {
    bit<1> tmp_1;
    bit<1> tmp_2;
    bit<1> z1;
    state start {
        z1 = 1w0;
        z = z1;
        tmp_1 = 1w0;
        tmp_2 = tmp_1 & 1w1;
        z = tmp_2;
        transition accept;
    }
}

m(p_0()) main;
