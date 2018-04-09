#include <core.p4>

control c(out bool x) {
    bool tmp_2;
    bool tmp_3;
    bool tmp_4;
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("c.t1") table t1 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("c.t2") table t2 {
        key = {
            x: exact @name("x") ;
        }
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action act() {
        tmp_2 = true;
    }
    @hidden action act_0() {
        tmp_2 = false;
    }
    @hidden action act_1() {
        x = true;
    }
    @hidden action act_2() {
        tmp_3 = false;
    }
    @hidden action act_3() {
        tmp_4 = true;
    }
    @hidden action act_4() {
        tmp_4 = false;
    }
    @hidden action act_5() {
        tmp_3 = tmp_4;
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
        if (t1.apply().hit) 
            tbl_act_0.apply();
        else 
            tbl_act_1.apply();
        if (!tmp_2) 
            tbl_act_2.apply();
        else {
            if (t2.apply().hit) 
                tbl_act_3.apply();
            else 
                tbl_act_4.apply();
            tbl_act_5.apply();
        }
        if (tmp_3) 
            tbl_act_6.apply();
    }
}

control proto(out bool x);
package top(proto p);
top(c()) main;

