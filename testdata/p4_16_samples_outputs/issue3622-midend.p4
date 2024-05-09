control c() {
    @name(".a") action a_0() {
    }
    @name(".b") action b_0() {
    }
    @name(".NoAction") action NoAction_1() {
    }
    @name("c.t") table t_0 {
        actions = {
            a_0();
            b_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        switch (t_0.apply().action_run) {
            a_0: {
            }
            default: {
            }
        }
    }
}

control C();
package top(C c);
top(c()) main;
