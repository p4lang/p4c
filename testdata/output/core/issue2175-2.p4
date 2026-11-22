extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    X() c1_x = {
        void a(inout bit<32> arg) {
            arg = arg + 1;
        }
    };
    apply {
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
