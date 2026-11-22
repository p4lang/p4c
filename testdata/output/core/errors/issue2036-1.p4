struct s {
    bit<8> x;
}

extern void f(in s sarg);
control c() {
    apply {
        tuple<bit<8>> b = { 0 };
        f(b);
    }
}

