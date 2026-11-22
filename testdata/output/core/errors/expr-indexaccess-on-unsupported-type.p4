extern bit<32> f(in bit<32> x);
control c() {
    apply {
        error h;
        h[f(2)].setValid();
    }
}

