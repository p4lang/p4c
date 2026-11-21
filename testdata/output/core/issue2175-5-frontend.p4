extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    @name("c.x") X() x_0 = {
        void a(inout bit<32> arg) {
            arg = arg + 32w1;
        }
    };
    apply {
        x_0.a(y);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(c()) main;
