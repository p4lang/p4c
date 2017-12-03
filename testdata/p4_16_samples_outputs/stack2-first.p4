#include <core.p4>

header h {
}

control c(out bit<32> x) {
    apply {
        h[4] stack;
        bit<32> sz = 32w4;
        x = sz;
    }
}

control Simpler(out bit<32> x);
package top(Simpler ctr);
top(c()) main;

