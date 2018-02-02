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
    h b_2;
    apply {
        stack_2[3].setValid();
        b_2 = stack_2[3];
        stack_2[2] = b_2;
        stack_2.push_front(2);
        stack_2.pop_front(2);
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;

