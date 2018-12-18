#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void func(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    Header[2] stack_0;
    bit<32> tmp;
    bit<32> tmp_1;
    state start {
        h.data1 = 32w0;
        func(h);
        tmp = h.data2;
        tmp_1 = g(tmp, h.data2);
        h.data2 = tmp;
        g(h.data2, tmp_1);
        h.data2 = h.data3 + 32w1;
        stack_0[1].isValid();
        transition select((bit<1>)h.isValid()) {
            1w1: next1;
            1w0: next2;
            default: noMatch;
        }
    }
    state next1 {
        transition next3;
    }
    state next2 {
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
    bit<32> e_0;
    @name("c.a1") action a1() {
    }
    @name("c.a1") action a1_2() {
    }
    @name("c.a2") action a2() {
    }
    @name("c.t") table t_0 {
        actions = {
            a1();
            a2();
        }
        default_action = a1();
    }
    @hidden action act() {
        e_0 = 32w1;
    }
    @hidden action act_0() {
        e_0 = e_0 + 32w1;
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
    @hidden table tbl_a1 {
        actions = {
            a1_2();
        }
        const default_action = a1_2();
    }
    apply {
        if (e_0 > 32w0) 
            tbl_act.apply();
        else 
            ;
        tbl_act_0.apply();
        switch (t_0.apply().action_run) {
            a1: {
            }
        }

        if (e_0 > 32w0) 
            t_0.apply();
        else 
            tbl_a1.apply();
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;

