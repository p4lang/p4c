extern E {
    E(bit<32> size);
}

control c() {
    E(12) e;
    apply {
    }
}

