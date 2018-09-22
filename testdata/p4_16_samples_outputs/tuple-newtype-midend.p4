#include <core.p4>

header H {
    bit<32> b;
}

struct tuple_0 {
    bit<1> field;
}

typedef tuple_0 T;
control c(out bit<32> x) {
    @hidden action act() {
        x = 32w0;
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

control e(out bit<32> x);
package top(e _e);
top(c()) main;

