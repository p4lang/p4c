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
    @hidden action simplify31() {
        x = true;
    }
    @hidden action act_1() {
        tmp_1 = true;
    }
    @hidden action act_2() {
        tmp_1 = false;
    }
    @hidden action simplify32() {
        tmp_0 = tmp_1;
    }
    @hidden action simplify32_0() {
        tmp_0 = false;
    }
    @hidden action simplify33() {
        x = false;
    }
    @hidden table tbl_simplify31 {
        actions = {
            simplify31();
        }
        const default_action = simplify31();
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
    @hidden table tbl_simplify32 {
        actions = {
            simplify32();
        }
        const default_action = simplify32();
    }
    @hidden table tbl_simplify32_0 {
        actions = {
            simplify32_0();
        }
        const default_action = simplify32_0();
    }
    @hidden table tbl_simplify33 {
        actions = {
            simplify33();
        }
        const default_action = simplify33();
    }
    apply {
        tbl_simplify31.apply();
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
            tbl_simplify32.apply();
        } else {
            tbl_simplify32_0.apply();
        }
        if (tmp_0) {
            tbl_simplify33.apply();
        }
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
