#include <core.p4>

struct h_t {
    bit<1> b;
}

control c(inout h_t h) {
    action a() {
    }
    table t {
        key = {
            h.b: exact;
        }
        actions = {
            a;
        }
        const entries = {
        }
    }
    apply {
    }
}

