#include <core.p4>

parser p(out bit<1> z) {
    bit<1> z1;
    state start {
        z1 = 1w0;
        z = z1;
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;

