extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    bit<32> x_0;
    Y(32w16) tmp;
    action act() {
        x_0 = tmp.get();
        x = x_0;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
