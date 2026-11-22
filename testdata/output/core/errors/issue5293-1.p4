header H {
}

control C(out bit<32> x) {
    H[{  }] stack;
    apply {
        stack = ...;
    }
}

