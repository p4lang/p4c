extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control c_0(out bit<32> x) {
    Y(32w16) inst;
    bit<32> tmp;
    apply {
        tmp = inst.get();
        x = tmp;
    }
}

control d(out bit<32> x) {
    bit<32> y_0;
    @name("cinst") c_0() cinst_0;
    apply {
        cinst_0.apply(x);
        cinst_0.apply(y_0);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;

