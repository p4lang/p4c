#include <core.p4>

header H1 {
    bit<32> x;
}

header H2 {
    bit<16> y;
}

struct Headers {
    H1 u_h1;
    H2 u_h2;
}

control c(in Headers h) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a") action a() {
    }
    @name("c.t") table t_0 {
        key = {
            h.u_h1.x: exact @name("h.u.h1.x");
        }
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

control _c(in Headers h);
package top(_c c);
top(c()) main;
