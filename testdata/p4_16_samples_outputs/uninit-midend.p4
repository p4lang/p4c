#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void func(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    Header[2] stack;
    bool c_1;
    bool d;
    bit<32> tmp_11;
    bit<32> tmp_12;
    bit<32> tmp_13;
    bit<32> tmp_14;
    bit<32> tmp_15;
    bit<32> tmp_16;
    state start {
        h.data1 = 32w0;
        func(h);
        tmp_11 = h.data2;
        tmp_12 = h.data2;
        tmp_13 = h.data2;
        tmp_14 = g(tmp_12, tmp_13);
        tmp_15 = tmp_14;
        g(tmp_11, tmp_15);
        tmp_16 = h.data3 + 32w1;
        h.data2 = tmp_16;
        stack[1].isValid();
        transition select(h.isValid()) {
            true: next1;
            false: next2;
            default: noMatch;
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
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control c(out bit<32> v) {
    bit<32> b;
    bit<32> d_2;
    bit<32> setByAction;
    bit<32> e;
    bool touched;
    bit<32> tmp_17;
    bit<32> tmp_18;
    bool tmp_19;
    bit<32> tmp_20;
    bool tmp_21;
    bit<32> tmp_22;
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
    }
    action act_0() {
        d_2 = 32w1;
        tmp_17 = b + 32w1;
        tmp_18 = 32w2;
        tmp_19 = e > 32w0;
    }
    action act_1() {
        touched = true;
    }
    action act_2() {
        tmp_20 = e + 32w1;
        e = e + 32w1;
    }
    action act_3() {
        tmp_21 = e > 32w0;
    }
    action act_4() {
        tmp_22 = setByAction + 32w1;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_2() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_3() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_a1() {
        actions = {
            a1_2();
        }
        const default_action = a1_2();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    apply {
        tbl_act.apply();
        if (e > 32w0) 
            tbl_act_0.apply();
        else 
            ;
        tbl_act_1.apply();
        switch (t.apply().action_run) {
            a1_0: {
                tbl_act_2.apply();
            }
        }

        tbl_act_3.apply();
        if (e > 32w0) 
            t.apply();
        else 
            tbl_a1.apply();
        tbl_act_4.apply();
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;
