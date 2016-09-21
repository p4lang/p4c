extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control c(out bit<32> x)(Y y) {
    bit<32> tmp;
    apply {
        tmp = y.get();
        x = tmp;
    }
}

control d(out bit<32> x) {
    @name("cinst") c(Y(32w16)) cinst_0;
    apply {
        cinst_0.apply(x);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
