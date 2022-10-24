#include <core.p4>

control E(out bit<1> b);
control Ingress(out bit<1> b) {
    @hidden action controlasparam13() {
        b = 1w0;
    }
    @hidden table tbl_controlasparam13 {
        actions = {
            controlasparam13();
        }
        const default_action = controlasparam13();
    }
    apply {
        tbl_controlasparam13.apply();
    }
}

package top(E _e);
top(Ingress()) main;
