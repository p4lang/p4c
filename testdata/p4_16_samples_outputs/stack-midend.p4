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
        stack.push_front(2);
        stack.pop_front(2);
        transition accept;
    }
}

parser Simple();
package top(Simple par);
top(p()) main;
