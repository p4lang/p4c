extern E {
    E(bit<32> size);
}

control c() {
    E(32w12) e;
    apply {
    }
}

