control proto(out bit<32> x);
package top(proto _c);
control c(out bit<32> x) {
    action act() {
        x = 32w0xf0f1e1e;
    }
    table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(c()) main;
