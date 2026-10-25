#include <core.p4>

struct meta_t {
    bit<16> m;
}

struct S {
    bit<16> f;
}

control C(inout meta_t meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.a") action a(@name("s") S s) {
        meta.m = 16w16;
    }
    @name("C.t") table t_0 {
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;
