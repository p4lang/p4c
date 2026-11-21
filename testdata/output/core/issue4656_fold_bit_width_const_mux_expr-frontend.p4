header h_t {
    bit<99> f;
}

control C() {
    @name("C.bar") action bar() {
    }
    @name("C.t") table t_0 {
        actions = {
            bar();
        }
        default_action = bar();
    }
    apply {
        t_0.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
