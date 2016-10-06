#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void func(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    @name("stack") Header[2] stack_0;
    @name("b") bool b_0;
    @name("c") bool c_0;
    @name("d") bool d_0;
    bit<32> tmp;
    bit<32> tmp_0;
    bit<32> tmp_1;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    state start {
        h.data1 = 32w0;
        func(h);
        tmp = h.data2;
        tmp_0 = h.data2;
        tmp_1 = h.data2;
        tmp_2 = g(tmp_0, tmp_1);
        h.data2 = tmp_0;
        tmp_3 = tmp_2;
        g(tmp, tmp_3);
        h.data2 = tmp;
        tmp_4 = h.data3 + 32w1;
        h.data2 = tmp_4;
        stack_0[0] = stack_0[1];
        b_0 = stack_0[1].isValid();
        transition select(h.isValid()) {
            true: next1;
            false: next2;
        }
    }
    state next1 {
        d_0 = false;
        transition next3;
    }
    state next2 {
        c_0 = true;
        d_0 = c_0;
        transition next3;
    }
    state next3 {
        c_0 = !c_0;
        d_0 = !d_0;
        transition accept;
    }
}

control c(out bit<32> v) {
    @name("b") bit<32> b_1;
    @name("d") bit<32> d_1;
    @name("setByAction") bit<32> setByAction_0;
    @name("e") bit<32> e_0;
    @name("f") bit<32> f_0;
    @name("touched") bool touched_0;
    @name("a1") action a1_0() {
        setByAction_0 = 32w1;
    }
    @name("a2") action a2_0() {
        setByAction_0 = 32w1;
    }
    @name("t") table t_0() {
        actions = {
            a1_0();
            a2_0();
        }
        default_action = a1_0();
    }
    bit<32> tmp_5;
    bit<32> tmp_6;
    bool tmp_7;
    bit<32> tmp_8;
    bool tmp_9;
    bit<32> tmp_10;
    apply {
        d_1 = 32w1;
        tmp_5 = b_1 + 32w1;
        b_1 = tmp_5;
        tmp_6 = d_1 + 32w1;
        d_1 = tmp_6;
        tmp_7 = e_0 > 32w0;
        if (tmp_7) {
            e_0 = 32w1;
            f_0 = 32w2;
        }
        else 
            f_0 = 32w3;
        tmp_8 = e_0 + 32w1;
        e_0 = tmp_8;
        switch (t_0.apply().action_run) {
            a1_0: {
                touched_0 = true;
            }
        }

        touched_0 = !touched_0;
        tmp_9 = e_0 > 32w0;
        if (tmp_9) 
            t_0.apply();
        else 
            a1_0();
        tmp_10 = setByAction_0 + 32w1;
        setByAction_0 = tmp_10;
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;
