#include <core.p4>

control ingress(inout bit<32> b) {
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bool tmp_1;
    @name("ingress.tmp_2") bit<8> tmp_2;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.t0") table t0_0 {
        key = {
            b: exact @name("b");
        }
        actions = {
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("ingress.t1") table t1_0 {
        key = {
            tmp_2: exact @name("key");
        }
        actions = {
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("ingress.t2") table t2_0 {
        key = {
            tmp_0: exact @name("key");
        }
        actions = {
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action act() {
        tmp_1 = true;
    }
    @hidden action act_0() {
        tmp_1 = false;
    }
    @hidden action issue25462l13() {
        tmp_2 = 8w1;
    }
    @hidden action issue25462l13_0() {
        tmp_2 = 8w2;
    }
    @hidden action act_1() {
        tmp = true;
    }
    @hidden action act_2() {
        tmp = false;
    }
    @hidden action issue25462l20() {
        tmp_0 = 8w3;
    }
    @hidden action issue25462l20_0() {
        tmp_0 = 8w4;
    }
    @hidden action issue25462l27() {
        b = 32w1;
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
    @hidden table tbl_issue25462l13 {
        actions = {
            issue25462l13();
        }
        const default_action = issue25462l13();
    }
    @hidden table tbl_issue25462l13_0 {
        actions = {
            issue25462l13_0();
        }
        const default_action = issue25462l13_0();
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
    @hidden table tbl_issue25462l20 {
        actions = {
            issue25462l20();
        }
        const default_action = issue25462l20();
    }
    @hidden table tbl_issue25462l20_0 {
        actions = {
            issue25462l20_0();
        }
        const default_action = issue25462l20_0();
    }
    @hidden table tbl_issue25462l27 {
        actions = {
            issue25462l27();
        }
        const default_action = issue25462l27();
    }
    apply {
        if (t0_0.apply().hit) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp_1) {
            tbl_issue25462l13.apply();
        } else {
            tbl_issue25462l13_0.apply();
        }
        if (t1_0.apply().hit) {
            tbl_act_1.apply();
        } else {
            tbl_act_2.apply();
        }
        if (tmp) {
            tbl_issue25462l20.apply();
        } else {
            tbl_issue25462l20_0.apply();
        }
        if (t2_0.apply().hit) {
            tbl_issue25462l27.apply();
        }
    }
}

control Ingress(inout bit<32> b);
package top(Ingress ig);
top(ingress()) main;
