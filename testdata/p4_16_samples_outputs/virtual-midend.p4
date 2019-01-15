struct data {
    bit<16> a;
    bit<16> b;
}

extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
    abstract void g(inout data ix);
}

control c(inout bit<16> p) {
    bit<16> tmp;
    @name("c.cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
        void g(inout data x) {
            data ix_0;
            ix_0.a = x.a;
            ix_0.b = x.b;
            if (x.a < x.b) 
                x.a = x.a + 16w1;
            if (ix_0.a > x.b) 
                x.a = ix_0.a + 16w65535;
        }
    };
    @hidden action act() {
        tmp = cntr_0.f(16w6);
        p = tmp;
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

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

