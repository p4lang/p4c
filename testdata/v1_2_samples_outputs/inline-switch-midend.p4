control d(out bit<32> x) {
    @name("x_0") bit<32> x_0_0;
    @name("hasReturned") bool hasReturned;
    @name("cinst.a1") action cinst_a1() {
    }
    @name("cinst.a2") action cinst_a2() {
    }
    @name("cinst.t") table cinst_t() {
        actions = {
            cinst_a1;
            cinst_a2;
        }
        default_action = cinst_a1;
    }
    action act() {
        hasReturned = false;
    }
    action act_0() {
        x = x_0_0;
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    apply {
        {
            tbl_act.apply();
            switch (cinst_t.apply().action_run) {
                cinst_a1: 
                cinst_a2: {
                    return;
                }
                default: {
                    return;
                }
            }

            ;
            tbl_act_0.apply();
        }
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
