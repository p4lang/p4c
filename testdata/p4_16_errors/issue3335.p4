parser MyParser1(bit tt1) {
    state start {
        transition select(1) {
                true: state1;
                _: accept;
        }
    }
    state state1 {
        transition accept;
    }
}
