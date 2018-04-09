extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x_0 = {
        void a(inout bit<32> arg) {
            bit<32> c1_tmp_1;
            bit<32> c1_tmp_2;
            c1_tmp_1 = this.b();
            c1_tmp_2 = arg + c1_tmp_1;
            arg = c1_tmp_2;
        }
    };
    @name("t.c2.x") X() c2_x_0 = {
        void a(inout bit<32> arg) {
            bit<32> c2_tmp_1;
            bit<32> c2_tmp_2;
            c2_tmp_1 = this.b();
            c2_tmp_2 = arg + c2_tmp_1;
            arg = c2_tmp_2;
        }
    };
    apply {
        c1_x_0.a(b);
        c2_x_0.a(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

