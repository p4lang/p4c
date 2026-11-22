header H {
    bit<32> b;
}

control I() {
    apply {
        H h = {b = 1};
        if (h == (H){b = { ... }}) {
            h = {b = 2};
        }
    }
}

