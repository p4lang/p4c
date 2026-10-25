struct s {
    bit<8> x;
}

extern void f(in s sarg);
control c() {
    apply {
        f({ true });
    }
}

