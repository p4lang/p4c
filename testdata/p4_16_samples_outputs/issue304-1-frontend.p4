extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control t(inout bit<32> b) {
    @name("t.c1.x") X() c1_x = {
        void a(inout bit<32> arg) {
            @name("t.c1.tmp") bit<32> c1_tmp;
            @name("t.c1.tmp_0") bit<32> c1_tmp_0;
            @name("t.c1.tmp_1") bit<32> c1_tmp_1;
            c1_tmp = arg;
            c1_tmp_0 = this.b();
            c1_tmp_1 = c1_tmp + c1_tmp_0;
            arg = c1_tmp_1;
        }
    };
    @name("t.c2.x") X() c2_x = {
        void a(inout bit<32> arg) {
            @name("t.c2.tmp") bit<32> c2_tmp;
            @name("t.c2.tmp_0") bit<32> c2_tmp_0;
            @name("t.c2.tmp_1") bit<32> c2_tmp_1;
            c2_tmp = arg;
            c2_tmp_0 = this.b();
            c2_tmp_1 = c2_tmp + c2_tmp_0;
            arg = c2_tmp_1;
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
