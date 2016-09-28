extern X {
    X();
}

control p() {
    X() x;
    apply {
        x.f();
    }
}

