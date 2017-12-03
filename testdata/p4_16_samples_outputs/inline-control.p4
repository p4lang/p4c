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
    const bit<32> arg = 32w16;
    c(Y(arg)) cinst;
    apply {
        cinst.apply(x);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

