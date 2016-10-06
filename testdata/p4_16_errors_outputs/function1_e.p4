extern void f(out bit<1> x);
control p() {
    apply {
        f(1w1 & 1w0);
    }
}

