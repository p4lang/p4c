#include <core.p4>

header h1_t {
    bit<8> f1;
}

struct S {
    h1_t h;
}

control C(inout S s, in bit<8> k, out bool v) {
    @name("C.a") action a() {
        s.h = (h1_t){#};
    }
    @name("C.a") action a_1() {
        s.h = (h1_t){#};
    }
    @name("C.t1") table t1_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    @name("C.t2") table t2_0 {
        key = {
            k: exact @name("k");
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    apply {
        t1_0.apply();
        v = s.h.isValid();
        t2_0.apply();
    }
}

control proto(inout S s, in bit<8> k, out bool v);
package top(proto p);
top(C()) main;
