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
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.a") action a() {
    }
    @name("c.t") table t_0 {
        key = {
            h.u.h1.x: exact @name("h.u.h1.x") ;
        }
        actions = {
            a();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t_0.apply();
    }
}

control _c(in Headers h);
package top(_c c);
top(c()) main;

