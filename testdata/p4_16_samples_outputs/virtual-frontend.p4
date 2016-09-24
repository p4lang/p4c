extern Virtual {
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    @name("cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            bit<16> tmp;
            tmp = ix + 16w1;
            return tmp;
        }
    };
    bit<16> tmp_0;
    apply {
        tmp_0 = cntr_0.f(16w6);
        p = tmp_0;
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
