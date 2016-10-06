header h {
}

parser p() {
    @name("stack") h[12] stack;
    @name("b") h b;
    state start {
        stack[0].setInvalid();
        stack[1].setInvalid();
        stack[2].setInvalid();
        stack[3].setInvalid();
        stack[4].setInvalid();
        stack[5].setInvalid();
        stack[6].setInvalid();
        stack[7].setInvalid();
        stack[8].setInvalid();
        stack[9].setInvalid();
        stack[10].setInvalid();
        stack[11].setInvalid();
        stack[3].setValid();
        b = stack[3];
        b = stack.last;
        stack[3] = b;
        b = stack.next;
        stack.push_front(2);
        stack.pop_front(2);
        transition accept;
    }
}

parser Simple();
package top(Simple par);
top(p()) main;
