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
        transition accept;
    }
}

control c() {
    @name("c.stack") h[4] stack_1;
    @hidden action stack38() {
        stack_1[0].setInvalid();
        stack_1[1].setInvalid();
        stack_1[2].setInvalid();
        stack_1[3].setInvalid();
        stack_1[3].setValid();
        stack_1[2] = stack_1[3];
        stack_1.push_front(2);
        stack_1.pop_front(2);
    }
    @hidden table tbl_stack38 {
        actions = {
            stack38();
        }
        const default_action = stack38();
    }
    apply {
        tbl_stack38.apply();
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;
