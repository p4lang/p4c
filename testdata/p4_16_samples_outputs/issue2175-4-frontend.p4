struct data {
    bit<16> a;
}

extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
    abstract void g(inout data ix);
}

control c(inout bit<16> p) {
    @name("c.cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
        void g(inout data x) {
        }
    };
    apply {
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
