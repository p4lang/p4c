header h {
}

parser p() {
    state start {
        h[12] stack;
        stack[3].setValid();
        h b = stack[3];
        b = stack.last;
        stack[3] = b;
        b = stack.next;
        stack.push_front(2);
        stack.pop_front(2);
    }
}

package top(p par);
top(p()) main;
