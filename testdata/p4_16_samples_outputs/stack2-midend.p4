#include <core.p4>

header h {
}

control c(out bit<32> x) {
    @hidden action act() {
        x = 32w4;
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

control Simpler(out bit<32> x);
package top(Simpler ctr);
top(c()) main;

