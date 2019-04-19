extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    X() x = {
        void a(inout bit<32> arg) {
            arg = arg + this.b();
        }
    };
    apply {
        x.a(y);
    }
}

control t(inout bit<32> b) {
    c() c1;
    c() c2;
    apply {
        c1.apply(b);
        c2.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

