action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

struct standard_metadata_t {
    bool ingress_port;
}

control ingress(inout standard_metadata_t sm) {
    table t2 {
        key = {
            sm.ingress_port: exact;
        }
        actions = {
        }
        const entries = {
                        0 : NoAction;
        }
    }
    apply {
    }
}

