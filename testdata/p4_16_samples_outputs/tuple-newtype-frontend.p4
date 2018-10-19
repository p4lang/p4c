#include <core.p4>

header H {
    bit<32> b;
}

type tuple<bit<1>> T;
control c(out bit<32> x) {
    T tt2_0;
    apply {
        tt2_0 = (T){ 1w0 };
        x = 32w0;
    }
}

control e(out bit<32> x);
package top(e _e);
top(c()) main;

