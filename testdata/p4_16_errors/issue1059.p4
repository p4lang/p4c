struct X {}

control c() {
    action a(in bit<32> z) {
    }

    apply {
        X x;
        a(x.x);
    }
}
