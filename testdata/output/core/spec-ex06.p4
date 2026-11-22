extern void f(in int<9> x, in int<9> y);
control p() {
    action A1() {
        f(9s4, 9s3);
    }
    apply {
    }
}

