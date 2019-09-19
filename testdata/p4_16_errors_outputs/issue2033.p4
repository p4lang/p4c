match_kind {
    exact,
    ternary,
    lpm
}

struct Header_t {
    bit<8> fieldname;
}

control ingress(Header_t h) {
    action nop() {
    }
    table badtable {
        default_action = nop;
        actions = {
            nop;
        }
    }
    apply {
        badtable.apply();
    }
}

