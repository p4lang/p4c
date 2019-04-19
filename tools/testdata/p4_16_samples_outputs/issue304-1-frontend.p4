extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x = {
        void a(inout bit<32> arg) {
            bit<32> c1_tmp;
            bit<32> c1_tmp_0;
            c1_tmp = this.b();
            c1_tmp_0 = arg + c1_tmp;
            arg = c1_tmp_0;
        }
    };
    @name("t.c2.x") X() c2_x = {
        void a(inout bit<32> arg) {
            bit<32> c2_tmp;
            bit<32> c2_tmp_0;
            c2_tmp = this.b();
            c2_tmp_0 = arg + c2_tmp;
            arg = c2_tmp_0;
        }
    };
    apply {
        c1_x.a(b);
        c2_x.a(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

