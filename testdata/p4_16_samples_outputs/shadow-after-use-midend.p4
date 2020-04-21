#include <core.p4>

control c(inout bit<16> x) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("c.incx") action incx() {
        x = x + 16w1;
    }
    @name("c.nop") action nop() {
    }
    @name("c.x") table x_0 {
        actions = {
            incx();
            nop();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        x_0.apply();
    }
}

control C(inout bit<16> x);
package top(C _c);
top(c()) main;

