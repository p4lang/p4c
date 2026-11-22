#include <core.p4>

control c(inout bit<16> x) {
    action incx() {
        x = x + 16w1;
    }
    action nop() {
    }
    table x {
        actions = {
            incx();
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        x.apply();
    }
}

control C(inout bit<16> x);
package top(C _c);
top(c()) main;
