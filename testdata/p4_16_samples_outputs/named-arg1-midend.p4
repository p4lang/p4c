#include <core.p4>

parser par(out bool b) {
    bit<32> x;
    bit<32> y;
    bit<32> x_4;
    state start {
        y = 32w0;
        x_4 = y + 32w6;
        x = x_4;
        b = x == 32w0;
        transition accept;
    }
}

control c(out bool b) {
    bit<16> xv;
    bool b_3;
    @name("c.a") action a_0() {
        xv = 16w65533;
    }
    @name("c.a") action a_2() {
        xv = 16w0;
    }
    @hidden action act() {
        b = xv == 16w0;
        b_3 = xv == 16w1;
        b = b_3;
        xv = 16w1;
        xv = 16w1;
        b = false;
        b_3 = true;
        b = true;
        xv = 16w1;
    }
    @hidden table tbl_a {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    @hidden table tbl_a_0 {
        actions = {
            a_2();
        }
        const default_action = a_2();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_a.apply();
        tbl_a_0.apply();
        tbl_act.apply();
    }
}

control ce(out bool b);
parser pe(out bool b);
package top(pe _p, ce _e, @optional ce _e1);
top(_e = c(), _p = par()) main;

