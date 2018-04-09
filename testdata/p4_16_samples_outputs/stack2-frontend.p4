#include <core.p4>

header h {
}

control c(out bit<32> x) {
    bit<32> sz;
    apply {
        sz = 32w4;
        x = sz;
    }
}

control Simpler(out bit<32> x);
package top(Simpler ctr);
top(c()) main;

