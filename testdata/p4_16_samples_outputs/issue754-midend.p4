control ctrl(out bit<3> _x);
package top(ctrl c);
control c_0(out bit<3> x) {
    @hidden action act() {
        x = 3w1;
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

top(c_0()) main;

