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
        stack_0.push_front(2);
        stack_0.pop_front(2);
        transition accept;
    }
}

parser Simple();
package top(Simple par);
top(p()) main;
