extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.y") bit<32> y_0;
    @name("d.x_0") bit<32> x_0;
    @name("d.cinst.y") Y(32w16) cinst_y;
    apply {
        x_0 = cinst_y.get();
        x = x_0;
        x_0 = cinst_y.get();
        y_0 = x_0;
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

