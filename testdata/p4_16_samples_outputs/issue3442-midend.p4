#include <core.p4>

control c();
package p(c c);
control myc() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("myc.a") action a() {
    }
    @name("myc.t") table t_0 {
        key = {
        }
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        const entries = {
                        default : a();
        }
        const default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

p(myc()) main;

