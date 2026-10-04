#include <core.p4>

header h {
}

control c(out bit<32> x) {
    @hidden action stack2l8() {
        x = 32w4;
    }
    @hidden table tbl_stack2l8 {
        actions = {
            stack2l8();
        }
        const default_action = stack2l8();
    }
    apply {
        tbl_stack2l8.apply();
    }
}

control Simpler(out bit<32> x);
package top(Simpler ctr);
top(c()) main;
