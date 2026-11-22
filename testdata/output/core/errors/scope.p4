extern X {
    X();
    bit<8> get();
}

control q() {
    X() x;
    apply {
    }
}

control r() {
    apply {
        bit<8> y = .q.x.get();
    }
}

