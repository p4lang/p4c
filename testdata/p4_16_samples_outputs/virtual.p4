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
    Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            return ix + 1;
        }
        void g(inout data x) {
            data ix = x;
            if (ix.a < ix.b) {
                x.a = ix.a + 1;
            }
            if (ix.a > ix.b) {
                x.a = ix.a - 1;
            }
        }
    };
    apply {
        p = cntr.f(6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

