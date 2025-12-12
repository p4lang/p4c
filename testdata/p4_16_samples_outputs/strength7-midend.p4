#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    int<8>  b3;
}

struct headers_t {
    t1 head;
}

extern bit<32> fn(in bit<32> x);
control c(inout headers_t hdrs) {
    @name("c.tmp_0") bit<32> tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    @name("c.act") action act() {
        fn(32w1);
    }
    @name("c.act") action act_1() {
        fn(32w3);
    }
    @name("c.act") action act_2() {
        fn(32w5);
    }
    @name("c.act") action act_3() {
        fn(32w7);
    }
    @name("c.act") action act_4() {
        fn(32w9);
    }
    @name("c.act") action act_5() {
        fn(32w11);
    }
    @name("c.act") action act_6() {
        fn(32w13);
    }
    @name("c.act") action act_7() {
        fn(32w15);
    }
    @name("c.act") action act_8() {
        fn(32w17);
    }
    @name("c.act") action act_9() {
        fn(32w19);
    }
    @name("c.act") action act_10() {
        fn(32w21);
    }
    @name("c.act") action act_11() {
        fn(32w23);
    }
    @hidden action act_0() {
        tmp_0 = fn(32w2);
        tmp_1 = fn(32w2);
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
    @hidden table tbl_act_7 {
        actions = {
            act_7();
        }
        const default_action = act_7();
    }
    @hidden table tbl_act_8 {
        actions = {
            act_8();
        }
        const default_action = act_8();
    }
    @hidden table tbl_act_9 {
        actions = {
            act_9();
        }
        const default_action = act_9();
    }
    @hidden table tbl_act_10 {
        actions = {
            act_10();
        }
        const default_action = act_10();
    }
    @hidden table tbl_act_11 {
        actions = {
            act_11();
        }
        const default_action = act_11();
    }
    apply {
        tbl_act.apply();
        tbl_act_0.apply();
        if (tmp_0 == tmp_1) {
            tbl_act_1.apply();
        }
        tbl_act_2.apply();
        tbl_act_3.apply();
        tbl_act_4.apply();
        tbl_act_5.apply();
        tbl_act_6.apply();
        tbl_act_7.apply();
        tbl_act_8.apply();
        tbl_act_9.apply();
        tbl_act_10.apply();
        tbl_act_11.apply();
    }
}

top<headers_t>(c()) main;
