extern E {
    E(bit<1> x);
}

control c() {
    E(true) e;
    apply {
    }
}

