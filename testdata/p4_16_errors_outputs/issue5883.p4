header H {
}

control C() {
    H[-0] stack;
    H[1] stack2;
    apply {
        stack = stack2;
        stack.pop_front(1);
    }
}

