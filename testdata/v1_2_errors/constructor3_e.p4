// incorrect arguments passed to constructor
extern E {
    E(bit x);
}

control c() {
    E(true) e;
    apply {}
}
