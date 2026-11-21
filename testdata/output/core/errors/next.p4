header h {
    bit<32> field;
}

control c() {
    h[10] stack;
    apply {
        stack.last = { 10 };
    }
}

