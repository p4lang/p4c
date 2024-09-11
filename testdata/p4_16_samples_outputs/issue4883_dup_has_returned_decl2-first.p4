#include <core.p4>

struct S {
    bit<16> f;
}

extern bool foo(in bool x, in bool y);
control C(inout S s) {
    action a1() {
        if (foo(s.f == 16w0, false)) {
            return;
        }
    }
    @name("a2") action __xyz() {
        a1();
        a1();
    }
    table t {
        actions = {
            __xyz();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control C2() {
    @name("C") C() C_inst;
    apply {
        S s;
        C_inst.apply(s);
    }
}

control proto();
package top(proto p);
top(C2()) main;
