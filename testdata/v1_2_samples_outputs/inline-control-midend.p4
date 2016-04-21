extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control d(out bit<32> x) {
    @name("x_0") bit<32> x_0_0;
    Y(32w16) tmp;
    apply {
        bool hasExited = false;
        x_0_0 = tmp.get();
        x = x_0_0;
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
