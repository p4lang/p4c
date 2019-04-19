#include <core.p4>

header h {
}

parser p() {
    h[4] stack_0;
    h b_0;
    state start {
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        stack_0[2].setInvalid();
        stack_0[3].setInvalid();
        stack_0[3].setValid();
        b_0 = stack_0.last;
        stack_0[2] = b_0;
        transition accept;
    }
}

control c() {
    h[4] stack_1;
    h b_1;
    apply {
        stack_1[3].setValid();
        b_1 = stack_1[3];
        stack_1[2] = b_1;
        stack_1.push_front(2);
        stack_1.pop_front(2);
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;

