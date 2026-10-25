extern bit<32> f(in bit<32> x);
control c() {
    apply {
        tuple<> h;
        h[f(2)].setValid();
    }
}

