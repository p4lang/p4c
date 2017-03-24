#include <core.p4>

control c(out bool x) {
    bool tmp_2;
    bool tmp_3;
    bool tmp_4;
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_3() {
    }
    @name("t1") table t1 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("t2") table t2 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    action act() {
        tmp_3 = false;
    }
    action act_0() {
        tmp_4 = t2.apply().hit;
        tmp_3 = tmp_4;
    }
    action act_1() {
        x = true;
        tmp_2 = t1.apply().hit;
    }
    action act_2() {
        x = false;
    }
    table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        if (!tmp_2) 
            tbl_act_0.apply();
        else {
            tbl_act_1.apply();
        }
        if (tmp_3) 
            tbl_act_2.apply();
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;
