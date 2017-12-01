control c() {
    @name("a") action a_0() {
    }
    @name("b") action b_0() {
    }
    @name("t1") table t1_0 {
        actions = {
            a_0();
            b_0();
        }
        default_action = a_0();
    }
    @name("t2") table t2_0 {
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    apply {
        t1_0.apply();
        t2_0.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;

