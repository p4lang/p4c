parser p() {
    state start {
        bit<8> x = 5;
        transition select(x, x, ..., x) {
            (0, 0, { 0, 0 }, 0): accept;
        }
    }
}

