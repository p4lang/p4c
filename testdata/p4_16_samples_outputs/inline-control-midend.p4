extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    bit<32> cinst_tmp;
    @name("d.cinst.y") Y(32w16) cinst_y;
    @hidden action act() {
        cinst_tmp = cinst_y.get();
        x = cinst_tmp;
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

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

