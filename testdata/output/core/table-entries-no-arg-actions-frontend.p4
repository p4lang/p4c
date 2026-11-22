#include <core.p4>

control C(inout bit<2> x);
package S(C c);
control MyC(inout bit<2> x) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyC.a") action a() {
    }
    @name("MyC.b") action b() {
    }
    @name("MyC.t") table t_0 {
        key = {
            x: exact @name("x");
        }
        actions = {
            a();
            b();
            @defaultonly NoAction_1();
        }
        const entries = {
                        2w0 : a();
                        2w1 : b();
                        2w2 : a();
                        2w3 : b();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

S(MyC()) main;
