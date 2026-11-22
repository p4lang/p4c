parser MyParser1(bit<1> tt1) {
    state start {
        transition select(1) {
            true: state1;
            default: accept;
        }
    }
    state state1 {
        transition accept;
    }
}

