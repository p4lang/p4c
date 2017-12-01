#include <core.p4>

header h {
}

parser p() {
    h[4] stack;
    h b;
    state start {
        stack[0].setInvalid();
        stack[1].setInvalid();
        stack[2].setInvalid();
        stack[3].setInvalid();
        stack[3].setValid();
        b = stack.last;
        stack[2] = b;
        transition accept;
    }
}

control c() {
    h[4] stack_2;
    @hidden action act() {
        stack_2[3].setValid();
        stack_2[2] = stack_2[3];
        stack_2.push_front(2);
        stack_2.pop_front(2);
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

