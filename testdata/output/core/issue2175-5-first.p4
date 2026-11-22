extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    X() x = {
        void a(inout bit<32> arg) {
            arg = arg + 32w1;
        }
    };
    apply {
        x.a(y);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(c()) main;
