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
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_0") bit<32> tmp_0;
    @name("c.tmp_1") bit<32> tmp_1;
    @name("c.tmp_2") bool tmp_2;
    @name("c.x") bit<32> x_0;
    @name("c.x") bit<32> x_13;
    @name("c.x") bit<32> x_14;
    @name("c.x") bit<32> x_15;
    @name("c.x") bit<32> x_16;
    @name("c.x") bit<32> x_17;
    @name("c.x") bit<32> x_18;
    @name("c.x") bit<32> x_19;
    @name("c.x") bit<32> x_20;
    @name("c.x") bit<32> x_21;
    @name("c.x") bit<32> x_22;
    @name("c.x") bit<32> x_23;
    @name("c.act") action act() {
        x_0 = 32w1;
        fn(x_0);
    }
    @name("c.act") action act_1() {
        x_13 = 32w3;
        fn(x_13);
    }
    @name("c.act") action act_2() {
        x_14 = 32w5;
        fn(x_14);
    }
    @name("c.act") action act_3() {
        x_15 = 32w7;
        fn(x_15);
    }
    @name("c.act") action act_4() {
        x_16 = 32w9;
        fn(x_16);
    }
    @name("c.act") action act_5() {
        x_17 = 32w11;
        fn(x_17);
    }
    @name("c.act") action act_6() {
        x_18 = 32w13;
        fn(x_18);
    }
    @name("c.act") action act_7() {
        x_19 = 32w15;
        fn(x_19);
    }
    @name("c.act") action act_8() {
        x_20 = 32w17;
        fn(x_20);
    }
    @name("c.act") action act_9() {
        x_21 = 32w19;
        fn(x_21);
    }
    @name("c.act") action act_10() {
        x_22 = 32w21;
        fn(x_22);
    }
    @name("c.act") action act_11() {
        x_23 = 32w23;
        fn(x_23);
    }
    apply {
        act();
        tmp_0 = fn(32w2);
        tmp = tmp_0;
        tmp_1 = fn(32w2);
        tmp_2 = tmp == tmp_1;
        if (tmp_2) {
            act_1();
        }
        act_2();
        act_3();
        act_4();
        act_5();
        act_6();
        act_7();
        act_8();
        act_9();
        act_10();
        act_11();
    }
}

top<headers_t>(c()) main;
