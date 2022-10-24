#include <core.p4>

parser par(out bool b) {
    state start {
        b = false;
        transition accept;
    }
}

control c(out bool b) {
    @name("c.a") action a() {
    }
    @name("c.a") action a_1() {
    }
    @hidden action act() {
        b = true;
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
