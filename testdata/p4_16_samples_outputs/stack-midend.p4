#include <core.p4>

header h {
}

parser p() {
    @name("p.stack") h[4] stack_0;
    state start {
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        stack_0[2].setInvalid();
        stack_0[3].setInvalid();
        stack_0[3].setValid();
        stack_0[2] = stack_0.last;
        transition accept;
    }
}

control c() {
    @name("c.stack") h[4] stack_1;
    @hidden action stack39() {
        stack_1[3].setValid();
        stack_1[2] = stack_1[3];
        stack_1.push_front(2);
        stack_1.pop_front(2);
    }
    @hidden table tbl_stack39 {
        actions = {
            stack39();
        }
        const default_action = stack39();
    }
    apply {
        tbl_stack39.apply();
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;

