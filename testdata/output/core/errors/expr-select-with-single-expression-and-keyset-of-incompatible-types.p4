parser p(in bit<16> x) {
    state start {
        transition select(x) {
            (4w2 .. 0): accept;
        }
    }
}

