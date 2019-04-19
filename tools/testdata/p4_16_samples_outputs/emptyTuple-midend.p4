struct tuple_0 {
}

typedef tuple_0 emptyTuple;
control c(out bool b) {
    @hidden action act() {
        b = true;
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

control e(out bool b);
package top(e _e);
top(c()) main;

