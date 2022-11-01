extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.cinst.y") Y(32w16) cinst_y;
    @hidden action inlinecontrol24() {
        x = cinst_y.get();
    }
    @hidden table tbl_inlinecontrol24 {
        actions = {
            inlinecontrol24();
        }
        const default_action = inlinecontrol24();
    }
    apply {
        tbl_inlinecontrol24.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
