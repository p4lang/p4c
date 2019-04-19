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
            ix_0 = x;
            if (ix_0.a < ix_0.b) 
                x.a = ix_0.a + 16w1;
            if (ix_0.a > ix_0.b) 
                x.a = ix_0.a + 16w65535;
        }
    };
    apply {
        tmp = cntr_0.f(16w6);
        p = tmp;
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

