#include <core.p4>

struct Headers {
    bit<8> a;
}

control ingress(inout Headers h) {
    action a1(bit<8> v) {
        h.a = v;
    }
    table t1 {
        key = {
            h.a: exact;
        }
        actions = {
            a1;
            NoAction;
        }
        const entries = {
                        1 .. 8 : a1(1);
        }
    }
    apply {
    }
}

