#include <core.p4>

header h {
}

parser p() {
    state start {
        transition accept;
    }
}

control c()(bit<32> x) {
    apply {
        h[4] s1;
        h[s1.size + 1] s2;
        h[x] s3;
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c(32w1)) main;

