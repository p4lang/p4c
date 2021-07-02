#include <core.p4>

parser par(out bool b) {
    state start {
        b = false;
        transition accept;
    }
}

control c(out bool b) {
    @name("c.xv") bit<16> xv_0;
    @name("c.b_1") bool b_1;
    @name("c.a") action a() {
        xv_0 = 16w65533;
    }
    @name("c.a") action a_1() {
        xv_0 = 16w0;
    }
    @hidden action namedarg1l22() {
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
            a_1();
        }
        const default_action = a_1();
    }
    @hidden table tbl_namedarg1l22 {
        actions = {
            namedarg1l22();
        }
        const default_action = namedarg1l22();
    }
    apply {
        tbl_a.apply();
        tbl_a_0.apply();
        tbl_namedarg1l22.apply();
    }
}

control ce(out bool b);
parser pe(out bool b);
package top(pe _p, ce _e, @optional ce _e1);
top(_e = c(), _p = par()) main;

