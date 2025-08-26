header H {
}

control C() {
    H[2] stack;
    apply {
        tuple<> h0;
        stack = { h0, ... };
    }
}

