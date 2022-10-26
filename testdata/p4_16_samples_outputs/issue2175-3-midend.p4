extern X {
    X();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1_x") X() c1_x_0 = {
        void a(inout bit<32> arg1) {
            arg1 = arg1 + 32w1;
        }
    };
    @name("t.c2_x") X() c2_x_0 = {
        void a(inout bit<32> arg2) {
            arg2 = arg2 + 32w2;
        }
    };
    apply {
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;
