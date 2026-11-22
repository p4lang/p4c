extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("d.cinst.y") Y(32w16) cinst_y;
    apply {
        x = cinst_y.get();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
