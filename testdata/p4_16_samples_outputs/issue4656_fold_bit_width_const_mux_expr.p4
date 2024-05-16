header h_t {
    bit<((10 == 1 ? 1 : 10 + 1) * (8 + 1))> f;
}

control C() {
    action bar() {
    }
    table t {
        actions = {
            bar;
        }
        default_action = bar;
    }
    apply {
        t.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
