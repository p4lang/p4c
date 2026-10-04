control C() {
    action empty() {
    }
    action set_value(inout bit<8> x) {
        x = 8w1;
    }
    apply {
        bit<8> x;
        set_value(x);
    }
}
