extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x = {
        void a(inout bit<32> arg) {
            @name("t.c1.tmp_0") bit<32> c1_tmp_0;
            c1_tmp_0 = this.b();
            arg = arg + c1_tmp_0;
        }
    };
    @name("t.c2.x") X() c2_x = {
        void a(inout bit<32> arg) {
            @name("t.c2.tmp_0") bit<32> c2_tmp_0;
            c2_tmp_0 = this.b();
            arg = arg + c2_tmp_0;
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
