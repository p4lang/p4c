extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    bit<16> tmp;
    @name("cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
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

