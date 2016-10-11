#include <core.p4>

extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp_2;
    bool tmp_3;
    bit<32> tmp_4;
    @name("r") bit<32> r_0;
    @name("a") action a_0() {
    }
    @name("b") action b_0() {
    }
    @name("t") table t() {
        key = {
            r_0: ternary;
        }
        actions = {
            a_0();
            b_0();
        }
        default_action = a_0();
    }
    action act() {
        r = 32w1;
    }
    action act_0() {
        r = 32w3;
    }
    action act_1() {
        tmp_2 = f(32w2);
        tmp_3 = tmp_2 < 32w2;
    }
    action act_2() {
        tmp_4 = f(32w2);
        r_0 = tmp_4;
    }
    table tbl_act() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_0() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_1() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_2() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        switch (t.apply().action_run) {
            a_0: {
                tbl_act_0.apply();
                if (tmp_3) 
                    tbl_act_1.apply();
                else 
                    tbl_act_2.apply();
            }
        }

    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
