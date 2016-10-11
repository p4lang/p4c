#include <core.p4>

control c(out bool x) {
    bool tmp_0;
    @name("NoAction_1") action NoAction() {
    }
    @name("NoAction_2") action NoAction_0() {
    }
    @name("t1") table t1() {
        key = {
            x: exact;
        }
        actions = {
            NoAction();
        }
        default_action = NoAction();
    }
    @name("t2") table t2() {
        key = {
            x: exact;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    action act() {
        tmp_0 = false;
    }
    action act_0() {
        tmp_0 = t2.apply().hit;
    }
    action act_1() {
        x = true;
    }
    action act_2() {
        x = false;
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
        if (!t1.apply().hit) 
            tbl_act_0.apply();
        else 
            tbl_act_1.apply();
        if (tmp_0) 
            tbl_act_2.apply();
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
