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
    @hidden action issue25462l19() {
        tmp_1 = true;
    }
    @hidden action issue25462l19_0() {
        tmp_1 = false;
    }
    @hidden action issue25462l19_1() {
        tmp_2 = 8w1;
    }
    @hidden action issue25462l19_2() {
        tmp_2 = 8w2;
    }
    @hidden action issue25462l26() {
        tmp = true;
    }
    @hidden action issue25462l26_0() {
        tmp = false;
    }
    @hidden action issue25462l26_1() {
        tmp_0 = 8w3;
    }
    @hidden action issue25462l26_2() {
        tmp_0 = 8w4;
    }
    @hidden action issue25462l33() {
        b = 32w1;
    }
    @hidden table tbl_issue25462l19 {
        actions = {
            issue25462l19();
        }
        const default_action = issue25462l19();
    }
    @hidden table tbl_issue25462l19_0 {
        actions = {
            issue25462l19_0();
        }
        const default_action = issue25462l19_0();
    }
    @hidden table tbl_issue25462l19_1 {
        actions = {
            issue25462l19_1();
        }
        const default_action = issue25462l19_1();
    }
    @hidden table tbl_issue25462l19_2 {
        actions = {
            issue25462l19_2();
        }
        const default_action = issue25462l19_2();
    }
    @hidden table tbl_issue25462l26 {
        actions = {
            issue25462l26();
        }
        const default_action = issue25462l26();
    }
    @hidden table tbl_issue25462l26_0 {
        actions = {
            issue25462l26_0();
        }
        const default_action = issue25462l26_0();
    }
    @hidden table tbl_issue25462l26_1 {
        actions = {
            issue25462l26_1();
        }
        const default_action = issue25462l26_1();
    }
    @hidden table tbl_issue25462l26_2 {
        actions = {
            issue25462l26_2();
        }
        const default_action = issue25462l26_2();
    }
    @hidden table tbl_issue25462l33 {
        actions = {
            issue25462l33();
        }
        const default_action = issue25462l33();
    }
    apply {
        if (t0_0.apply().hit) {
            tbl_issue25462l19.apply();
        } else {
            tbl_issue25462l19_0.apply();
        }
        if (tmp_1) {
            tbl_issue25462l19_1.apply();
        } else {
            tbl_issue25462l19_2.apply();
        }
        if (t1_0.apply().hit) {
            tbl_issue25462l26.apply();
        } else {
            tbl_issue25462l26_0.apply();
        }
        if (tmp) {
            tbl_issue25462l26_1.apply();
        } else {
            tbl_issue25462l26_2.apply();
        }
        if (t2_0.apply().hit) {
            tbl_issue25462l33.apply();
        }
    }
}

control Ingress(inout bit<32> b);
package top(Ingress ig);
top(ingress()) main;
