extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x_0 = {
        void a(inout bit<32> arg) {
            bit<32> c1_tmp_1;
            c1_tmp_1 = this.b();
            arg = arg + c1_tmp_1;
        }
    };
    @name("t.c2.x") X() c2_x_0 = {
        void a(inout bit<32> arg) {
            bit<32> c2_tmp_1;
            c2_tmp_1 = this.b();
            arg = arg + c2_tmp_1;
        }
    };
    @hidden action act() {
        c1_x_0.a(b);
        c2_x_0.a(b);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

