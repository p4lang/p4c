extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    bit<32> y;
    bit<32> x_1;
    @name("d.cinst.inst") Y(32w16) cinst_inst_0;
    bit<32> cinst_tmp_0;
    apply {
        cinst_tmp_0 = cinst_inst_0.get();
        x_1 = cinst_tmp_0;
        x = x_1;
        cinst_tmp_0 = cinst_inst_0.get();
        x_1 = cinst_tmp_0;
        y = x_1;
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

