header h_t {
    bit<99> f;
}

control C() {
    action bar() {
    }
    table t {
        actions = {
            bar();
        }
        default_action = bar();
    }
    apply {
        t.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
