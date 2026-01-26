struct Headers {
    bool h1;
}

parser P(out Headers h) {
    state start {
        transition select(h.h1.last.x) {
        }
    }
}

