header H {
}

control C() {
    H[2] stack;
    apply {
        stack = (H[2]){ 0, ... };
    }
}

