extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    X() c1_x = {
        void a(inout bit<32> arg1) {
            arg1 = arg1 + 1;
        }
    };
    X() c2_x = {
        void a(inout bit<32> arg2) {
            arg2 = arg2 + 2;
        }
    };
    apply {
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
