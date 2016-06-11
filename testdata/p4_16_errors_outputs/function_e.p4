extern void f(in bit<1> x);
extern void g<T>();
control p() {
    apply {
        f();
        f(1w1, 1w0);
        f<bit<1>>(1w0);
        g<bit<1>, bit<1>>();
    }
}

