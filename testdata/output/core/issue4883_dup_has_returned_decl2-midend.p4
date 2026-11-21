#include <core.p4>

struct S {
    bit<16> f;
}

extern bool foo(in bool x, in bool y);
control C2() {
    @name("C2.s") S s_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C2.C.a2") action C_a2_0() {
        foo(s_0.f == 16w0, false);
        foo(s_0.f == 16w0, false);
    }
    @name("C2.C.t") table C_t {
        actions = {
            C_a2_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        C_t.apply();
    }
}

control proto();
package top(proto p);
top(C2()) main;
