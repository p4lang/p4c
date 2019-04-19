#include <core.p4>

header H1 {
    bit<32> x;
}

header H2 {
    bit<16> y;
}

header_union U {
    H1 h1;
    H2 h2;
}

struct Headers {
    U u;
}

control c(in Headers h) {
    action a() {
    }
    table t {
        key = {
            h.u.h1.x: exact;
        }
        actions = {
            a;
        }
    }
    apply {
        t.apply();
    }
}

control _c(in Headers h);
package top(_c c);
top(c()) main;

