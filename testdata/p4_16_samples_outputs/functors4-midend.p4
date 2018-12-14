#include <core.p4>

parser p(out bit<1> z) {
    state start {
        z = 1w0;
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;

