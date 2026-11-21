enum bit<8> myenum {
    value = 8w0
}

parser MyParser1(in myenum a) {
    state start {
        transition select(a) {
            8w1 .. 8w22: state1;
            default: accept;
        }
    }
    state state1 {
        transition accept;
    }
}

