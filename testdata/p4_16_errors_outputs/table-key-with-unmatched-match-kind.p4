control ingress() {
    bit<4> y;
    table X {
        key = {
            y: TT_FWD;
        }
        actions = {
        }
    }
    apply {
    }
}

