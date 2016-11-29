extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    bit<32> tmp_0;
    @name("cinst.inst") Y(32w16) cinst_inst_0;
    action act() {
        tmp_0 = cinst_inst_0.get();
        x = tmp_0;
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
