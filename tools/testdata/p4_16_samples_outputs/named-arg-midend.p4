extern void f(in bit<16> x, in bool y);
control c() {
    @hidden action act() {
        f(y = true, x = 16w0);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control empty();
package top(empty _e);
top(c()) main;

