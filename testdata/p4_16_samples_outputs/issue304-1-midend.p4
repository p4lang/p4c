extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("c1.x") X() c1_x_0 = {
        void a(inout bit<32> arg) {
            @name("c1.tmp") bit<32> c1_tmp_1;
            @name("c1.tmp_0") bit<32> c1_tmp_2;
            c1_tmp_1 = this.b();
            c1_tmp_2 = arg + c1_tmp_1;
        }
    };
    @name("c2.x") X() c2_x_0 = {
        void a(inout bit<32> arg) {
            @name("c2.tmp") bit<32> c2_tmp_1;
            @name("c2.tmp_0") bit<32> c2_tmp_2;
            c2_tmp_1 = this.b();
            c2_tmp_2 = arg + c2_tmp_1;
        }
    };
    action act() {
        c1_x_0.a(b);
        c2_x_0.a(b);
    }
    table tbl_act() {
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
