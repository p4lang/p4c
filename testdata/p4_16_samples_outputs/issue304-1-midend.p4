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
    @hidden action act() {
        c1_x.a(b);
        c2_x.a(b);
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

