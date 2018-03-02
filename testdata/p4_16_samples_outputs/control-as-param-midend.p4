#include <core.p4>

control E(out bit<1> b);
control Ingress(out bit<1> b) {
    @hidden action act() {
        b = 1w1;
        b = 1w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

package top(E _e);
top(Ingress()) main;

