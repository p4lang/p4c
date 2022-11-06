#include <core.p4>

control c() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("c.a") action a() {
    }
    @name("c.a") action a_1() {
    }
    @name("c.t1") table t1_0 {
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("c.t2") table t2_0 {
        actions = {
            a_1();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        t1_0.apply();
        t2_0.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
