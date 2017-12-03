extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control c(out bit<32> x)(Y y) {
    apply {
        x = y.get();
    }
}

control d(out bit<32> x) {
    bit<32> y;
    const bit<32> arg = 32w16;
    c(Y(32w16)) cinst;
    apply {
        y = 32w5;
        cinst.apply(x);
        cinst.apply(y);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

