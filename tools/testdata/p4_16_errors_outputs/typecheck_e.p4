extern void f(in int<32> d);
control p<T>(in T x) {
    apply {
        f(x);
    }
}

