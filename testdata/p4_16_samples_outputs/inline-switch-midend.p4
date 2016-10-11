control d(out bit<32> x) {
    bit<32> x_1;
    bool hasReturned_0;
    @name("cinst.a1") action cinst_a1() {
    }
    @name("cinst.a2") action cinst_a2() {
    }
    @name("cinst.t") table cinst_t_0() {
        actions = {
            cinst_a1();
            cinst_a2();
        }
        default_action = cinst_a1();
    }
    action act() {
        hasReturned_0 = true;
    }
    action act_0() {
        hasReturned_0 = true;
    }
    action act_1() {
        hasReturned_0 = false;
    }
    action act_2() {
        x = x_1;
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
        switch (cinst_t_0.apply().action_run) {
            cinst_a1: 
            cinst_a2: {
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
