header h {
}

parser p() {
    h[12] stack_0;
    h b_0;
    state start {
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        stack_0[2].setInvalid();
        stack_0[3].setInvalid();
        stack_0[4].setInvalid();
        stack_0[5].setInvalid();
        stack_0[6].setInvalid();
        stack_0[7].setInvalid();
        stack_0[8].setInvalid();
        stack_0[9].setInvalid();
        stack_0[10].setInvalid();
        stack_0[11].setInvalid();
        stack_0[3].setValid();
        b_0 = stack_0[3];
        b_0 = stack_0.last;
        stack_0[3] = b_0;
        b_0 = stack_0.next;
        stack_0.push_front(2);
        stack_0.pop_front(2);
        transition accept;
    }
}

parser Simple();
package top(Simple par);
top(p()) main;
