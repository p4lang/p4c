extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.x_0") bit<32> x_0;
    @name("d.cinst.y") Y(32w16) cinst_y;
    @hidden action inlinecontrol1l24() {
        x_0 = cinst_y.get();
        x = x_0;
        cinst_y.get();
    }
    @hidden table tbl_inlinecontrol1l24 {
        actions = {
            inlinecontrol1l24();
        }
        const default_action = inlinecontrol1l24();
    }
    apply {
        tbl_inlinecontrol1l24.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
