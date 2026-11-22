#include <core.p4>

struct meta_t {
    bit<16> m;
}

struct S {
    bit<16> f;
}

control C(inout meta_t meta) {
    action a(S s) {
        meta.m = s.minSizeInBits();
    }
    table t {
        actions = {
            a;
        }
    }
    apply {
        t.apply();
    }
}

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;
