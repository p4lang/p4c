extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("x_0") bit<32> x_1;
    @name("tmp") bit<32> tmp_2;
    @name("tmp_0") Y(32w16) tmp_1;
    action act() {
        tmp_2 = tmp_1.get();
        x_1 = tmp_2;
        x = x_1;
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
