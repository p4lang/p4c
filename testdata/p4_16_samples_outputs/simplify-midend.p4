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
    @hidden action simplify23() {
        tmp = true;
    }
    @hidden action simplify23_0() {
        tmp = false;
    }
    @hidden action simplify22() {
        x = true;
    }
    @hidden action simplify23_1() {
        tmp_1 = true;
    }
    @hidden action simplify23_2() {
        tmp_1 = false;
    }
    @hidden action simplify23_3() {
        tmp_0 = tmp_1;
    }
    @hidden action simplify23_4() {
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
    @hidden table tbl_simplify23_1 {
        actions = {
            simplify23_1();
        }
        const default_action = simplify23_1();
    }
    @hidden table tbl_simplify23_2 {
        actions = {
            simplify23_2();
        }
        const default_action = simplify23_2();
    }
    @hidden table tbl_simplify23_3 {
        actions = {
            simplify23_3();
        }
        const default_action = simplify23_3();
    }
    @hidden table tbl_simplify23_4 {
        actions = {
            simplify23_4();
        }
        const default_action = simplify23_4();
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
            tbl_simplify23.apply();
        } else {
            tbl_simplify23_0.apply();
        }
        if (tmp) {
            if (t2_0.apply().hit) {
                tbl_simplify23_1.apply();
            } else {
                tbl_simplify23_2.apply();
            }
            tbl_simplify23_3.apply();
        } else {
            tbl_simplify23_4.apply();
        }
        if (tmp_0) {
            tbl_simplify24.apply();
        }
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
