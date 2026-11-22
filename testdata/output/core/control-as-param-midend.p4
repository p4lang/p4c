#include <core.p4>

control E(out bit<1> b);
control Ingress(out bit<1> b) {
    @hidden action controlasparam19() {
        b = 1w0;
    }
    @hidden table tbl_controlasparam19 {
        actions = {
            controlasparam19();
        }
        const default_action = controlasparam19();
    }
    apply {
        tbl_controlasparam19.apply();
    }
}

package top(E _e);
top(Ingress()) main;
