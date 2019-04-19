control c(in bit<1> b) {
    action NoAction() {
    }
    table t {
        key = {
            b: noSuchMatch;
        }
        actions = {
            NoAction;
        }
    }
    apply {
    }
}

