#include <core.p4>

header h {
}

parser p() {
    h[4] stack_0;
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
    h[4] stack_1;
    @hidden action act() {
        stack_1[3].setValid();
        stack_1[2] = stack_1[3];
        stack_1.push_front(2);
        stack_1.pop_front(2);
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

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;

