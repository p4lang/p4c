enum bit<2> foo_t {
    A = 0
}

control c0() {
    action set_x(tuple<> v) {
    }
    table t {
        actions = {
            set_x;
        }
        default_action = set_x(foo_t.A);
    }
    apply {
    }
}

