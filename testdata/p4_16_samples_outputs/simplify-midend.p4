#include <core.p4>

control c(out bool x) {
    @name("c.tmp") bool tmp;
    @name("c.tmp_0") bool tmp_0;
    @name("c.tmp_1") bool tmp_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("c.t1") table t1_0 {
        key = {
            x: exact @name("x");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("c.t2") table t2_0 {
        key = {
            x: exact @name("x");
        }
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @hidden action act() {
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action simplify22() {
        x = true;
    }
    @hidden action act_1() {
        tmp_1 = true;
    }
    @hidden action act_2() {
        tmp_1 = false;
    }
    @hidden action simplify23() {
        tmp_0 = tmp_1;
    }
    @hidden action simplify23_0() {
        tmp_0 = false;
    }
    @hidden action simplify24() {
        x = false;
    }
    @hidden table tbl_simplify22 {
        actions = {
            simplify22();
        }
        const default_action = simplify22();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_simplify23 {
        actions = {
            simplify23();
        }
        const default_action = simplify23();
    }
    @hidden table tbl_simplify23_0 {
        actions = {
            simplify23_0();
        }
        const default_action = simplify23_0();
    }
    @hidden table tbl_simplify24 {
        actions = {
            simplify24();
        }
        const default_action = simplify24();
    }
    apply {
        tbl_simplify22.apply();
        if (t1_0.apply().hit) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp) {
            if (t2_0.apply().hit) {
                tbl_act_1.apply();
            } else {
                tbl_act_2.apply();
            }
            tbl_simplify23.apply();
        } else {
            tbl_simplify23_0.apply();
        }
        if (tmp_0) {
            tbl_simplify24.apply();
        }
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
