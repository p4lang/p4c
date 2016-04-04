header h {
}

control p() {
    apply {
        h[12] stack;
        h b = stack[3];
        b = stack.last;
        b = stack.next;
        stack[3] = b;
        stack.push_front(2);
        stack.pop_front(2);
    }
}

