control c(out bit<32> x) {
    @name("a1") action a1_0() {
    }
    @name("a2") action a2_0() {
    }
    @name("t") table t_0 {
        actions = {
            a1_0();
            a2_0();
        }
        default_action = a1_0();
    }
    apply {
        switch (t_0.apply().action_run) {
            a1_0: 
            a2_0: {
                return;
            }
            default: {
                return;
            }
        }

    }
}

control d(out bit<32> x) {
    @name("cinst") c() cinst_0;
    apply {
        cinst_0.apply(x);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

