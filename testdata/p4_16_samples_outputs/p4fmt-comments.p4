control C() {
    action empty() {
    }
    action set_value(inout bit<8> x) {
        x = 1;
    }
    apply {
        bit<8> x;
        set_value(x);
    }
}
