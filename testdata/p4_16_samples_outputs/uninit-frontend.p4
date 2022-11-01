#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

extern void func(in Header h);
extern bit<32> g(inout bit<32> v, in bit<32> w);
parser p1(packet_in p, out Header h) {
    @name("p1.stack") Header[2] stack_0;
    @name("p1.tmp") bit<32> tmp;
    @name("p1.tmp_0") bit<32> tmp_0;
    @name("p1.tmp_1") bit<32> tmp_1;
    state start {
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        h.data1 = 32w0;
        func(h);
        tmp = h.data2;
        tmp_0 = h.data2;
        tmp_1 = g(h.data2, tmp_0);
        g(tmp, tmp_1);
        h.data2 = h.data3 + 32w1;
        transition select(h.isValid()) {
            true: next1;
            false: next2;
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
}

control c(out bit<32> v) {
    @name("c.e") bit<32> e_0;
    @name("c.a1") action a1() {
    }
    @name("c.a1") action a1_1() {
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
    apply {
        if (e_0 > 32w0) {
            e_0 = 32w1;
        } else {
            ;
        }
        e_0 = e_0 + 32w1;
        switch (t_0.apply().action_run) {
            a1: {
            }
            default: {
            }
        }
        if (e_0 > 32w0) {
            t_0.apply();
        } else {
            a1_1();
        }
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;
