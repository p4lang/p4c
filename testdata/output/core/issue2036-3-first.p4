struct s {
    bit<8> x;
}

extern void f(in tuple<bit<8>> a, in s sarg);
control c() {
    apply {
        f({ 8w0 }, (s){x = 8w0});
    }
}

