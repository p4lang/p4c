extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    bit<16> tmp_0;
    @name("c.cntr") Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
    };
    apply {
        tmp_0 = cntr.f(16w6);
        p = tmp_0;
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

