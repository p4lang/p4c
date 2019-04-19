#include <core.p4>

parser par(out bool b) {
    state start {
        b = false;
        transition accept;
    }
}

control c(out bool b) {
    bit<16> xv_0;
    bool b_1;
    @name("c.a") action a() {
        xv_0 = 16w65533;
    }
    @name("c.a") action a_2() {
        xv_0 = 16w0;
    }
    @hidden action act() {
        b = xv_0 == 16w0;
        b_1 = xv_0 == 16w1;
        b = b_1;
        xv_0 = 16w1;
        xv_0 = 16w1;
        b = false;
        b_1 = true;
        b = true;
        xv_0 = 16w1;
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
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

