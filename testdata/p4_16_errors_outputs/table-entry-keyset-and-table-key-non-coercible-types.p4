match_kind {
    exact,
    ternary,
    lpm
}

header H {
    bit<32> e;
    bit<32> t;
}

struct Headers {
    H h;
}

control c(in Headers h) {
    action a_params(bit<32> param) {
    }
    table t_exact_ternary {
        key = {
            h.h.e: exact;
            h.h.t: ternary;
        }
        actions = {
        }
        entries = {
                        const priority=10: (1, false) : a_params(1);
        }
    }
    apply {
    }
}

