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
    bool b;
    bool c;
    bool d;
    state start {
        h.data1 = 32w0;
        func(h);
        g(h.data2, g(h.data2, h.data2));
        transition next;
    }
    state next {
        h.data2 = h.data3 + 32w1;
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
        c = true;
        d = c;
        transition next3;
    }
    state next3 {
        c = !c;
        d = !d;
        transition accept;
    }
}

control c(out bit<32> v) {
    bit<32> b;
    bit<32> d = 32w1;
    bit<32> setByAction;
    action a1() {
        setByAction = 32w1;
    }
    action a2() {
        setByAction = 32w1;
    }
    table t {
        actions = {
            a1();
            a2();
        }
        default_action = a1();
    }
    apply {
        b = b + 32w1;
        d = d + 32w1;
        bit<32> e;
        bit<32> f;
        if (e > 32w0) {
            e = 32w1;
            f = 32w2;
        }
        else 
            f = 32w3;
        e = e + 32w1;
        bool touched;
        switch (t.apply().action_run) {
            a1: {
                touched = true;
            }
        }

        touched = !touched;
        if (e > 32w0) 
            t.apply();
        else 
            a1();
        setByAction = setByAction + 32w1;
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;

