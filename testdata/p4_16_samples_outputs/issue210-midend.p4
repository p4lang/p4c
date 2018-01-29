#include <core.p4>

control Ing(out bit<32> a) {
    bool b;
    @name("Ing.cond") action cond_0() {
        a = (b ? 32w5 : a);
        a = (!b ? 32w10 : a);
    }
    @hidden action act() {
        b = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_cond {
        actions = {
            cond_0();
        }
        const default_action = cond_0();
    }
    apply {
        tbl_act.apply();
        tbl_cond.apply();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;

