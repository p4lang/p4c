control d(out bit<32> x) {
    bit<32> x_0;
    bool hasReturned;
    @name("cinst.a1") action cinst_a1_0() {
    }
    @name("cinst.a2") action cinst_a2_0() {
    }
    @name("cinst.t") table cinst_t() {
        actions = {
            cinst_a1_0();
            cinst_a2_0();
        }
        default_action = cinst_a1_0();
    }
    action act() {
        hasReturned = true;
    }
    action act_0() {
        hasReturned = true;
    }
    action act_1() {
        hasReturned = false;
    }
    action act_2() {
        x = x_0;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_2() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        switch (cinst_t.apply().action_run) {
            cinst_a1_0: 
            cinst_a2_0: {
                tbl_act_0.apply();
            }
            default: {
                tbl_act_1.apply();
            }
        }

        tbl_act_2.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
