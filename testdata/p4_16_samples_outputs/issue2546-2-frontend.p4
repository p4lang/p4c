#include <core.p4>

control ingress(inout bit<32> b) {
    @name("ingress.key_1") bit<8> key_0;
    @name("ingress.key_0") bit<8> key_1;
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
            key_0: exact @name("key");
        }
        actions = {
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("ingress.t2") table t2_0 {
        key = {
            key_1: exact @name("key");
        }
        actions = {
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        tmp_1 = t0_0.apply().hit;
        if (tmp_1) {
            tmp_2 = 8w1;
        } else {
            tmp_2 = 8w2;
        }
        key_0 = tmp_2;
        tmp = t1_0.apply().hit;
        if (tmp) {
            tmp_0 = 8w3;
        } else {
            tmp_0 = 8w4;
        }
        key_1 = tmp_0;
        if (t2_0.apply().hit) {
            b = 32w1;
        }
    }
}

control Ingress(inout bit<32> b);
package top(Ingress ig);
top(ingress()) main;
