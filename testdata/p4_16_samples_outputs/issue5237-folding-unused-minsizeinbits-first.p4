#include <core.p4>

struct meta_t {
    bit<16> m;
}

struct S {
    bit<16> f;
}

control C(inout meta_t meta) {
    action a(S s) {
        meta.m = 16w16;
    }
    table t {
        actions = {
            a();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;
