header H {
}

struct S1 {
}

control C(out bit<32> x) {
    H[{  }] stack;
    apply {
        H h0 = ...;
        stack = { h0, ... };
    }
}

