control c() {
    @name("c.a") action a_0() {
    }
    @name("c.a") action a_2() {
    }
    @name("c.b") action b_0() {
    }
    @name("c.t1") table t1 {
        actions = {
            a_0();
            b_0();
        }
        default_action = a_0();
    }
    @name("c.t2") table t2 {
        actions = {
            a_2();
        }
        default_action = a_2();
    }
    apply {
        t1.apply();
        t2.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;

