#include <core.p4>

control c(out bool x) {
    bool tmp;
    bool tmp_0;
    bool tmp_1;
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("c.t1") table t1_0 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("c.t2") table t2_0 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action act() {
        tmp = true;
    }
    @hidden action act_0() {
        tmp = false;
    }
    @hidden action act_1() {
        x = true;
    }
    @hidden action act_2() {
        tmp_0 = false;
    }
    @hidden action act_3() {
        tmp_1 = true;
    }
    @hidden action act_4() {
        tmp_1 = false;
    }
    @hidden action act_5() {
        tmp_0 = tmp_1;
    }
    @hidden action act_6() {
        x = false;
    }
    @hidden table tbl_act {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_act_5 {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    @hidden table tbl_act_6 {
        actions = {
            act_6();
        }
        const default_action = act_6();
    }
    apply {
        tbl_act.apply();
        if (t1_0.apply().hit) 
            tbl_act_0.apply();
        else 
            tbl_act_1.apply();
        if (!tmp) 
            tbl_act_2.apply();
        else {
            if (t2_0.apply().hit) 
                tbl_act_3.apply();
            else 
                tbl_act_4.apply();
            tbl_act_5.apply();
        }
        if (tmp_0) 
            tbl_act_6.apply();
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;

