#include <core.p4>

control C(inout bit<2> x);
package S(C c);
control MyC(inout bit<2> x) {
    action a() {
    }
    action b() {
    }
    table t {
        key = {
            x: exact @name("x");
        }
        actions = {
            a();
            b();
            @defaultonly NoAction();
        }
        const entries = {
                        2w0 : a();
                        2w1 : b();
                        2w2 : a();
                        2w3 : b();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

S(MyC()) main;
