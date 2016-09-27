#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void func(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    @name("stack") Header[2] stack;
    @name("b") bool b;
    @name("c") bool c_1;
    @name("d") bool d;
    @name("tmp") bit<32> tmp_11;
    @name("tmp_0") bit<32> tmp_12;
    @name("tmp_1") bit<32> tmp_13;
    @name("tmp_2") bit<32> tmp_14;
    @name("tmp_3") bit<32> tmp_15;
    @name("tmp_4") bit<32> tmp_16;
    state start {
        h.data1 = 32w0;
        func(h);
        tmp_11 = h.data2;
        tmp_12 = h.data2;
        tmp_13 = h.data2;
        tmp_14 = g(tmp_12, tmp_13);
        h.data2 = tmp_12;
        tmp_15 = tmp_14;
        g(tmp_11, tmp_15);
        h.data2 = tmp_11;
        tmp_16 = h.data3 + 32w1;
        h.data2 = tmp_16;
        stack[0] = stack[1];
        b = stack[1].isValid();
        transition select(h.isValid()) {
            true: next1;
            false: next2;
        }
    }
    state next1 {
        d = false;
        transition next3;
    }
    state next2 {
        c_1 = true;
        d = c_1;
        transition next3;
    }
    state next3 {
        c_1 = !c_1;
        d = !d;
        transition accept;
    }
}

control c(out bit<32> v) {
    @name("b") bit<32> b_2;
    @name("d") bit<32> d_2;
    @name("setByAction") bit<32> setByAction;
    @name("e") bit<32> e;
    @name("f") bit<32> f;
    @name("touched") bool touched;
    @name("tmp_5") bit<32> tmp_17;
    @name("tmp_6") bit<32> tmp_18;
    @name("tmp_7") bool tmp_19;
    @name("tmp_8") bit<32> tmp_20;
    @name("tmp_9") bool tmp_21;
    @name("tmp_10") bit<32> tmp_22;
    @name("a1") action a1_0() {
        setByAction = 32w1;
    }
    @name("a1") action a1_2() {
        setByAction = 32w1;
    }
    @name("a2") action a2_0() {
        setByAction = 32w1;
    }
    @name("t") table t() {
        actions = {
            a1_0();
            a2_0();
        }
        default_action = a1_0();
    }
    action act() {
        e = 32w1;
        f = 32w2;
    }
    action act_0() {
        f = 32w3;
    }
    action act_1() {
        d_2 = 32w1;
        tmp_17 = b_2 + 32w1;
        b_2 = tmp_17;
        tmp_18 = d_2 + 32w1;
        d_2 = tmp_18;
        tmp_19 = e > 32w0;
    }
    action act_2() {
        touched = true;
    }
    action act_3() {
        tmp_20 = e + 32w1;
        e = tmp_20;
    }
    action act_4() {
        touched = !touched;
        tmp_21 = e > 32w0;
    }
    action act_5() {
        tmp_22 = setByAction + 32w1;
        setByAction = tmp_22;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_2() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_3() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    table tbl_a1() {
        actions = {
            a1_2();
        }
        const default_action = a1_2();
    }
    table tbl_act_5() {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    apply {
        tbl_act.apply();
        if (tmp_19) {
            tbl_act_0.apply();
        }
        else 
            tbl_act_1.apply();
        tbl_act_2.apply();
        switch (t.apply().action_run) {
            a1_0: {
                tbl_act_3.apply();
            }
        }

        tbl_act_4.apply();
        if (tmp_21) 
            t.apply();
        else 
            tbl_a1.apply();
        tbl_act_5.apply();
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;
