extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    bit<32> y;
    bit<32> x_1;
    bit<32> cinst_tmp_0;
    @name("cinst.inst") Y(32w16) cinst_inst_0;
    action act() {
        cinst_tmp_0 = cinst_inst_0.get();
        x_1 = cinst_tmp_0;
        x = cinst_tmp_0;
        cinst_tmp_0 = cinst_inst_0.get();
        x_1 = cinst_tmp_0;
        y = cinst_tmp_0;
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
