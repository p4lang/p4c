#include <core.p4>

control c(out bool x) {
    bool tmp;
    bool tmp_0;
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    @name("t1") table t1_0() {
        key = {
            x: exact;
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("t2") table t2_0() {
        key = {
            x: exact;
        }
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    action act() {
        x = false;
    }
    action act_0() {
        x = true;
        tmp = t1_0.apply().hit;
        tmp_0 = t2_0.apply().hit;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        if (tmp && tmp_0) 
            tbl_act_0.apply();
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
