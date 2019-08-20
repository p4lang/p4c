extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x = {
        void a(inout bit<32> arg) {
            bit<32> c1_tmp;
            c1_tmp = this.b();
            arg = arg + c1_tmp;
        }
    };
    @name("t.c2.x") X() c2_x = {
        void a(inout bit<32> arg) {
            bit<32> c2_tmp;
            c2_tmp = this.b();
            arg = arg + c2_tmp;
        }
    };
    @hidden action issue3041l28() {
        c1_x.a(b);
        c2_x.a(b);
    }
    @hidden table tbl_issue3041l28 {
        actions = {
            issue3041l28();
        }
        const default_action = issue3041l28();
    }
    apply {
        tbl_issue3041l28.apply();
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

