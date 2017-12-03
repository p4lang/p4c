extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    @name("x") X() x_0 = {
        void a(inout bit<32> arg) {
            bit<32> tmp;
            bit<32> tmp_0;
            tmp = this.b();
            tmp_0 = arg + tmp;
            arg = tmp_0;
        }
    };
    apply {
        x_0.a(y);
    }
}

control t(inout bit<32> b) {
    @name("c1") c() c1_0;
    @name("c2") c() c2_0;
    apply {
        c1_0.apply(b);
        c2_0.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

