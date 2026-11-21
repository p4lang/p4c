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
            x: exact;
        }
        actions = {
            a;
            b;
        }
        const entries = {
                        0 : a;
                        1 : b;
                        2 : a();
                        3 : b();
        }
    }
    apply {
        t.apply();
    }
}

S(MyC()) main;
