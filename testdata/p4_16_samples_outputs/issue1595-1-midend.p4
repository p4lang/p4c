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
    @hidden action act() {
        b[6:3] = 4w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        switch (t_0.apply().action_run) {
            a: {
                tbl_act.apply();
            }
        }

    }
}

control empty(inout bit<32> b);
package top(empty _e);
top(c()) main;

