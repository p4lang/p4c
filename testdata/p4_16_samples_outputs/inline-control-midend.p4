extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.cinst.y") Y(32w16) cinst_y;
    @hidden action inlinecontrol15() {
        x = cinst_y.get();
    }
    @hidden table tbl_inlinecontrol15 {
        actions = {
            inlinecontrol15();
        }
        const default_action = inlinecontrol15();
    }
    apply {
        tbl_inlinecontrol15.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
