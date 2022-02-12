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
    @hidden action uninit87() {
        e_0 = 32w1;
    }
    @hidden action uninit92() {
        e_0 = e_0 + 32w1;
    }
    @hidden table tbl_uninit87 {
        actions = {
            uninit87();
        }
        const default_action = uninit87();
    }
    @hidden table tbl_uninit92 {
        actions = {
            uninit92();
        }
        const default_action = uninit92();
    }
    @hidden table tbl_a1 {
        actions = {
            a1_1();
        }
        const default_action = a1_1();
    }
    apply {
        if (e_0 > 32w0) {
            tbl_uninit87.apply();
        } else {
            ;
        }
        tbl_uninit92.apply();
        switch (t_0.apply().action_run) {
            a1: {
            }
            default: {
            }
        }
        if (e_0 > 32w0) {
            t_0.apply();
        } else {
            tbl_a1.apply();
        }
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;

