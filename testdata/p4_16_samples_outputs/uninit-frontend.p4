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
    bit<32> tmp_3;
    bit<32> tmp_4;
    bit<32> tmp_5;
    bit<32> tmp_6;
    state start {
        h.data1 = 32w0;
        func(h);
        tmp_3 = h.data2;
        tmp_4 = h.data2;
        tmp_5 = g(tmp_3, tmp_4);
        h.data2 = tmp_3;
        tmp_6 = tmp_5;
        g(h.data2, tmp_6);
        h.data2 = h.data3 + 32w1;
        stack[1].isValid();
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
        transition accept;
    }
}

control c(out bit<32> v) {
    bit<32> d_2;
    bit<32> setByAction;
    bit<32> e;
    bool touched;
    @name("c.a1") action a1_0() {
        setByAction = 32w1;
    }
    @name("c.a1") action a1_2() {
        setByAction = 32w1;
    }
    @name("c.a2") action a2_0() {
        setByAction = 32w1;
    }
    @name("c.t") table t {
        actions = {
            a1_0();
            a2_0();
        }
        default_action = a1_0();
    }
    apply {
        d_2 = 32w1;
        if (e > 32w0) 
            e = 32w1;
        else 
            ;
        e = e + 32w1;
        switch (t.apply().action_run) {
            a1_0: {
                touched = true;
            }
        }

        if (e > 32w0) 
            t.apply();
        else 
            a1_2();
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;

