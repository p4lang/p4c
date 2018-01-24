extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.cinst.inst") Y(32w16) cinst_inst_0;
    bit<32> cinst_tmp_0;
    apply {
        cinst_tmp_0 = cinst_inst_0.get();
        x = cinst_tmp_0;
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

