#include <core.p4>

control E(out bit<1> b);
control Ingress(out bit<1> b) {
    @hidden action controlasparam7() {
        b = 1w1;
        b = 1w0;
    }
    @hidden table tbl_controlasparam7 {
        actions = {
            controlasparam7();
        }
        const default_action = controlasparam7();
    }
    apply {
        tbl_controlasparam7.apply();
    }
}

package top(E _e);
top(Ingress()) main;

