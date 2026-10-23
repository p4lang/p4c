#include <core.p4>

match_kind {
    optional
}

struct meta_t {
    error e1;
    error e2;
}

control C(inout meta_t m);
package top(C c);
control MyControl(inout meta_t m) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("MyControl.noop") action noop() {
    }
    @name("MyControl.noop") action noop_1() {
    }
    @name("MyControl.t_exact") table t_exact_0 {
        key = {
            m.e1: exact @name("m.e1");
        }
        actions = {
            noop();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("MyControl.t_optional") table t_optional_0 {
        key = {
            m.e2: optional @name("m.e2");
        }
        actions = {
            noop_1();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        t_exact_0.apply();
        t_optional_0.apply();
    }
}

top(MyControl()) main;
