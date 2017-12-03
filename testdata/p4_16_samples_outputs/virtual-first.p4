extern Virtual {
    Virtual();
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            return ix + 16w1;
        }
    };
    apply {
        p = cntr.f(16w6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

