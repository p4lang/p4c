#include <core.p4>

control Ing(out bit<32> a) {
    bool b_0;
    @name("Ing.cond") action cond() {
        a = (b_0 ? 32w5 : a);
        a = (!b_0 ? 32w10 : a);
    }
    @hidden action act() {
        b_0 = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_cond {
        actions = {
            cond();
        }
        const default_action = cond();
    }
    apply {
        tbl_act.apply();
        tbl_cond.apply();
    }
}

control s(out bit<32> a);
package top(s e);
top(Ing()) main;

