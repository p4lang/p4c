control c(inout bit<32> b) {
    @name("c.a") action a() {
        b = 32w1;
    }
    @name("c.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        switch (t_0.apply().action_run) {
            a: {
                b[6:3] = 4w1;
            }
        }

    }
}

control empty(inout bit<32> b);
package top(empty _e);
top(c()) main;

